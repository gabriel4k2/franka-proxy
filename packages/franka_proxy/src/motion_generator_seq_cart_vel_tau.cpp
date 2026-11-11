/**
 *************************************************************************
 *
 * @file motion_generator_seq_cart_vel_tau.cpp
 *
 * ..., implementation.
 *
 ************************************************************************/


#include "motion_generator_seq_cart_vel_tau.hpp"

#include <iostream>
#include <fstream>
#include <utility>
#include <cstdio>
#include <exception>

#include <Eigen/Dense>

#include <franka/model.h>

#include "franka_proxy_util.hpp"


namespace franka_proxy::detail
{
//////////////////////////////////////////////////////////////////////////
//
// seq_cart_vel_tau_generator
//
//////////////////////////////////////////////////////////////////////////


seq_cart_vel_tau_generator::seq_cart_vel_tau_generator(
	std::mutex& current_state_lock,
	franka::RobotState& current_state,
	franka::Robot& robot,
	const std::atomic_bool& stop_motion_flag,
	std::vector<std::array<double, 7>> q_sequence,
	std::vector<std::array<double, 6>> f_sequence,
	std::vector<std::array<double, 6>> selection_vector_sequence,
	unsigned long long timing_id)
	: current_state_lock_(current_state_lock),
	  current_state_(current_state),
	  stop_motion_(stop_motion_flag),
	  model(robot.loadModel()),
	  timing_id_(timing_id),
	  q_sequence_(std::move(q_sequence)),
	  f_sequence_(std::move(f_sequence)),
	  selection_vector_sequence_(std::move(selection_vector_sequence)),
	  dq_buffer_(dq_filter_size_, eigen_vector7d::Zero()),
	  ft_buffer_(ft_filter_size_, Eigen::Matrix<double, 6, 1>::Zero()),
	  stiffness_(6, 6),
	  damping_(6, 6)
{
	stiffness_.setZero();
	stiffness_.topLeftCorner(3, 3) =
		translational_stiffness_ * Eigen::MatrixXd::Identity(3, 3);
	stiffness_.bottomRightCorner(3, 3) =
		rotational_stiffness_ * Eigen::MatrixXd::Identity(3, 3);

	// Underdamping with 1/4 for smoother motions on real robot
	damping_.setZero();
	damping_.topLeftCorner(3, 3) =
		0.25 * 2. * sqrt(translational_stiffness_) * Eigen::MatrixXd::Identity(3, 3);
	damping_.bottomRightCorner(3, 3) =
		0.25 * 2. * sqrt(rotational_stiffness_) * Eigen::MatrixXd::Identity(3, 3);
}


seq_cart_vel_tau_generator::~seq_cart_vel_tau_generator()
{
	// The control callback has returned (or unwound) before this summary is printed.
	std::fprintf(stderr,
		"[move_sequence] side=server batch=%llu phase=tracking outcome=%s unwinding=%d "
		"samples=%zu nominal_ms=%zu callbacks=%zu callback_elapsed_ms=%.3f progress_ms=%.3f "
		"last_step=%zu stalled_callbacks=%zu stall_episodes=%zu stall_ms=%.3f "
		"longest_stall_ms=%.3f clock_rewind_ms=%zu peak_error_mm=%.3f last_error_mm=%.3f "
		"max_period_ms=%.3f position_offset=%d\n",
		timing_id_, timing_outcome_, std::uncaught_exceptions() > 0,
		q_sequence_.size(), q_sequence_.size(), timing_.callbacks, timing_.elapsed_s * 1000., time_ * 1000.,
		timing_last_step_, timing_.stalled_callbacks, timing_.stall_episodes, timing_.stall_s * 1000.,
		timing_.longest_stall_s * 1000., timing_clock_rewinds_,
		timing_.peak_error_m * 1000., timing_.last_error_m * 1000., timing_.max_period_s * 1000.,
		timing_has_offset_);
	if (timing_have_target_)
	{
		const auto print_boundary = [&](const char* boundary, const auto& q, const auto& q_d, const auto& target) {
			std::fprintf(stderr, "[move_sequence] side=server batch=%llu phase=tracking boundary=%s",
				timing_id_, boundary);
			const auto print_joints = [](const char* name, const auto& joints) {
				std::fprintf(stderr, " %s=[", name);
				for (size_t i = 0; i < joints.size(); ++i)
					std::fprintf(stderr, "%s%.6f", i ? "," : "", joints[i]);
				std::fputc(']', stderr);
			};
			print_joints("measured_q", q);
			print_joints("robot_q_d", q_d);
			print_joints("sequence_q", target);
			std::fputc('\n', stderr);
		};
		print_boundary("start", timing_first_q_, timing_first_q_d_, q_sequence_.front());
		print_boundary("end", timing_last_q_, timing_last_q_d_, q_sequence_[timing_last_step_]);
	}
	std::fflush(stderr);

	if (log_)
	{
		try
		{
			std::ofstream csv("motion_log.csv");
			csv << "f0,f1,f2,t0,t1,t2,e0,e1,e2,e3,e4,e5\n";
			for (int i = 0; i < ft_existing_log_.size(); ++i)
				csv << ft_existing_log_[i][0] << ","
					<< ft_existing_log_[i][1] << ","
					<< ft_existing_log_[i][2] << ","
					<< ft_existing_log_[i][3] << ","
					<< ft_existing_log_[i][4] << ","
					<< ft_existing_log_[i][5] << ","
					<< error_log_[i][0] << ","
					<< error_log_[i][1] << ","
					<< error_log_[i][2] << ","
					<< error_log_[i][3] << ","
					<< error_log_[i][4] << ","
					<< error_log_[i][5] << '\n';
		}
		catch (const std::exception& e)
		{
			std::cerr << "seq_cart_vel_tau_generator::~seq_cart_vel_tau_generator()"
				<< "Exception during logging: " << e.what() << '\n';
		}
	}
}


franka::Torques seq_cart_vel_tau_generator::step(
	const franka::RobotState& robot_state,
	franka::Duration period,
	std::optional<std::array<double, 16>> offset_position,
	std::optional<std::array<double, 6>> offset_force)
{
	timing_.record_period(period.toSec());
	if (timing_.callbacks == 1)
	{
		timing_first_q_ = robot_state.q;
		timing_first_q_d_ = robot_state.q_d;
	}
	timing_last_q_ = robot_state.q;
	timing_last_q_d_ = robot_state.q_d;
	timing_has_offset_ = offset_position.has_value();

	{
		std::lock_guard state_guard(current_state_lock_);
		current_state_ = robot_state;
	}

	if (stop_motion_)
	{
		timing_outcome_ = "stopped";
		throw stop_motion_trigger();
	}


	bool contact_change_motion = false;

	time_ += period.toSec();
	bool motion_finished = false;
	auto current_step = static_cast<size_t>(time_ * 1000.);


	// q_start
	if (time_ == 0.0)
	{
		if ((eigen_vector7d(robot_state.q.data())
			- eigen_vector7d(q_sequence_.front().data())).norm() > 0.01)
		{
			timing_outcome_ = "start_pose_rejected";
			throw std::runtime_error("Aborting; too far away from starting pose!");
		}
	}

	// q_end
	if (current_step >= q_sequence_.size())
	{
		current_step = q_sequence_.size() - 1;
		motion_finished = true;
	}


	timing_last_step_ = current_step;
	timing_have_target_ = true;

	// get target variables
	std::array<double, 7> q_d = q_sequence_[current_step];
	std::array<double, 6> f_d = f_sequence_[current_step];
	std::array<double, 6> selection_vector = selection_vector_sequence_[current_step];


	// get state variables
	update_dq_filter(robot_state);
	Eigen::Map<const eigen_vector7d> tau_measured(robot_state.tau_J.data());

	std::array<double, 42> jacobian_array =
		model.zeroJacobian(franka::Frame::kEndEffector, robot_state);
	Eigen::Map<const Eigen::Matrix<double, 6, 7>> jacobian(jacobian_array.data());

	std::array<double, 49> mass_array = model.mass(robot_state);
	Eigen::Map<eigen_vector7d> mass(mass_array.data());
	std::array<double, 7> coriolis_array = model.coriolis(robot_state);
	Eigen::Map<eigen_vector7d> coriolis(coriolis_array.data());
	std::array<double, 7> gravity_array = model.gravity(robot_state);
	Eigen::Map<eigen_vector7d> gravity(gravity_array.data());


	// --- cartesian motion ---

	auto current_pose = model.pose(franka::Frame::kEndEffector, robot_state.q, robot_state.F_T_EE, robot_state.EE_T_K);
	Eigen::Affine3d transform(Eigen::Matrix4d::Map(current_pose.data()));
	//Eigen::Affine3d transform(Eigen::Matrix4d::Map(robot_state.O_T_EE.data()));
	Eigen::Vector3d position(transform.translation());
	Eigen::Quaterniond orientation(transform.linear());


	// calculate pose from desired joints 
	auto desired_pose = model.pose(franka::Frame::kEndEffector, q_d, robot_state.F_T_EE, robot_state.EE_T_K);
	if (offset_position)
		desired_pose = apply_pos_increment(desired_pose, offset_position.value());

	Eigen::Affine3d transform_d(Eigen::Matrix4d::Map(desired_pose.data()));
	Eigen::Vector3d position_d(transform_d.translation());
	Eigen::Quaterniond orientation_d(transform_d.linear());


	// compute error to desired pose
	Eigen::Matrix<double, 6, 1> error;

	// position error
	error.head(3) = position - position_d;

	// orientation error
	if (orientation_d.coeffs().dot(orientation.coeffs()) < 0.0)
		orientation.coeffs() = -orientation.coeffs();
	Eigen::Quaterniond error_quaternion(orientation.inverse() * orientation_d);
	error.tail(3) << error_quaternion.x(), error_quaternion.y(), error_quaternion.z();
	// transform to base frame
	error.tail(3) = -transform_d.linear() * error.tail(3);


	const double raw_translation_error = error.head(3).norm();
	if (error.head(3).norm() > 0.005 &&
		selection_vector[0] == 1 && selection_vector[1] == 1 && selection_vector[2] == 1)
	{
		error.head(3) = error.head(3).normalized() * 0.005;
		contact_change_motion = true;
	}
	timing_.record_tracking(raw_translation_error, contact_change_motion, period.toSec());


	// spring damper system with damping ratio=1 and filtered dq
	Eigen::Matrix<double, 6, 1> ft_cartesian_motion =
		-stiffness_ * error - damping_ * (jacobian * compute_dq_filtered());

	// --- cartesian motion end --- 


	// --- force motion ---

	Eigen::Matrix<double, 6, 1> ft_desired(f_d.data());

	// feedforward force
	Eigen::Matrix<double, 6, 1> ft_force = ft_desired;
	if (offset_force)
		ft_desired = apply_force_increment(ft_desired, offset_force.value());

	update_ft_filter(ft_force);
	ft_force = compute_ft_filtered();

	Eigen::Matrix<double, 6, 1> ft_task;
	for (int i = 0; i < 6; ++i)
		ft_task[i] = selection_vector[i] * ft_cartesian_motion[i] + (1 - selection_vector[i]) * ft_force[i];

	// --- force motion end ---


	// --- compute control ---
	Eigen::Matrix<double, 7, 1> tau_task = jacobian.transpose() * ft_task;
	Eigen::Matrix<double, 7, 1> tau_d = tau_task + coriolis;


	if (log_)
	{
		pose_log_.emplace_back(transform);
		pose_d_log_.emplace_back(transform_d);
		error_log_.emplace_back(error);
		ft_log_.emplace_back(ft_force);
		//ft_existing_log_.emplace_back(ft_existing);
	}

	if (contact_change_motion)
	{
		time_ -= 0.001; // period.toSec(); // stay at the same step 
		++timing_clock_rewinds_;
	}

	if (!franka_proxy_util::is_tau_within_percentage_of_max_limit(tau_d, 0.3))
	{
		timing_outcome_ = "torque_limit";
		throw std::runtime_error("Motion generator callback: Unreasonable tau at time " + std::to_string(time_));
	}

	std::array<double, 7> tau_d_array{};
	Eigen::VectorXd::Map(tau_d_array.data(), 7) = tau_d;
	franka::Torques output(tau_d_array);
	output.motion_finished = motion_finished;
	if (motion_finished)
		timing_outcome_ = "finished";
	return output;
}


void seq_cart_vel_tau_generator::update_dq_filter(const franka::RobotState& robot_state)
{
	dq_buffer_[dq_current_filter_position_] = eigen_vector7d(robot_state.dq.data());

	dq_current_filter_position_ = (dq_current_filter_position_ + 1) % dq_filter_size_;
}


Eigen::Matrix<double, 7, 1> seq_cart_vel_tau_generator::compute_dq_filtered() const
{
	eigen_vector7d value(eigen_vector7d::Zero());

	for (size_t i = 0; i < dq_filter_size_; ++i)
		value += dq_buffer_[i];

	return value / dq_filter_size_;
}


void seq_cart_vel_tau_generator::update_ft_filter(const Eigen::Matrix<double, 6, 1>& current_ft)
{
	ft_buffer_[ft_current_filter_position_] = current_ft;

	ft_current_filter_position_ = (ft_current_filter_position_ + 1) % ft_filter_size_;
}


Eigen::Matrix<double, 6, 1> seq_cart_vel_tau_generator::compute_ft_filtered() const
{
	Eigen::Matrix<double, 6, 1> value(Eigen::Matrix<double, 6, 1>::Zero());

	for (size_t i = 0; i < ft_filter_size_; ++i)
		value += ft_buffer_[i];

	return value / ft_filter_size_;
}


std::array<double, 16> seq_cart_vel_tau_generator::apply_pos_increment(
	const std::array<double, 16>& desired_pose,
	const std::array<double, 16>& increment)
{
	std::array<double, 16> result{};

	for (int col = 0; col < 4; ++col)
	{
		for (int row = 0; row < 4; ++row)
		{
			double sum = 0.0;
			for (int k = 0; k < 4; ++k)
				sum += increment[row + 4 * k] * desired_pose[k + 4 * col];
			result[row + 4 * col] = sum;
		}
	}

	return result;
}


Eigen::Matrix<double, 6, 1> seq_cart_vel_tau_generator::apply_force_increment(
	const Eigen::Matrix<double, 6, 1>& ft_desired,
	const std::array<double, 6>& increment)
{
	Eigen::Matrix<double, 6, 1> result = ft_desired;
	for (int i = 0; i < 6; i++)
		result(i) += increment[0];
	return result;
}
}
