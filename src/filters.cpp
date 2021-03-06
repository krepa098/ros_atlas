/*
 * ATLAS - Cooperative sensing
 * Copyright (C) 2017  Paul KREMER
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "filters.h"

/**
 * @brief WeightedMean
 */

WeightedMean::WeightedMean()
{
}

void WeightedMean::addVec3(const tf2::Vector3& vec, double weight)
{
    m_vectorWeightedSum += Eigen::Vector3d{ vec.x(), vec.y(), vec.z() } * weight;
    m_vectorWeights += weight;
}

void WeightedMean::addQuat(const tf2::Quaternion& quat, double weight)
{
    // quaternion weighted sum
    m_quats.conservativeResize(4, m_quats.cols() + 1);
    m_quats.col(m_quats.cols() - 1) = weight * Eigen::Vector4d{
        quat.x(),
        quat.y(),
        quat.z(),
        quat.w()
    };
}

void WeightedMean::reset()
{
    m_vectorWeightedSum = { 0, 0, 0 };
    m_vectorWeights     = 0.0;
    m_quats             = Eigen::Matrix4Xd();
}

tf2::Vector3 WeightedMean::weightedMeanVec3() const
{
    if (m_vectorWeights == 0.0)
        return { 0, 0, 0 };

    auto average = m_vectorWeightedSum / m_vectorWeights;
    return { average[0], average[1], average[2] };
}

tf2::Quaternion WeightedMean::weightedMeanQuat() const
{
    // quaternion interpolation matrix
    // calculations based on http://www.acsu.buffalo.edu/~johnc/ave_quat07.pdf

    // solve the eigenproblem
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix4Xd> solver(m_quats * m_quats.transpose());

    // find largest eigenvalue
    int index     = 0;
    double maxVal = -1.0;

    for (int i = 0; i < solver.eigenvalues().rows(); ++i)
    {
        if (std::abs(solver.eigenvalues()[i]) > maxVal)
        {
            maxVal = std::abs(solver.eigenvalues()[i]);
            index  = i;
        }
    }

    // the eigenvector corresponding to the largest eigenvalue
    // is the weighted average of the quaternion
    auto eigenvec = solver.eigenvectors().col(index);
    return { eigenvec[0], eigenvec[1], eigenvec[2], eigenvec[3] };
}

/**
 * @brief ExplonentialMovingAverage
 */

ExplonentialMovingAverageFilter::ExplonentialMovingAverageFilter()
{
}

ExplonentialMovingAverageFilter::ExplonentialMovingAverageFilter(double alpha, ros::Duration timeout)
    : m_alpha(alpha)
    , m_timeout(timeout)
{
}

void ExplonentialMovingAverageFilter::addScalar(double scalar)
{
    checkReinit();
    if (m_scalarInitialized)
    {
        m_scalarAccu = (m_alpha * scalar) + (1.0 - m_alpha) * m_scalarAccu;
    }
    else
    {
        m_scalarAccu        = scalar;
        m_scalarInitialized = true;
    }

    m_timeOfLastValue = ros::Time::now();
}

void ExplonentialMovingAverageFilter::addVec3(const tf2::Vector3& vec)
{
    checkReinit();
    if (m_vecInitialized)
    {
        m_vectorAccu = (m_alpha * vec) + (1.0 - m_alpha) * m_vectorAccu;
    }
    else
    {
        m_vectorAccu     = vec;
        m_vecInitialized = true;
    }

    m_timeOfLastValue = ros::Time::now();
}

void ExplonentialMovingAverageFilter::addQuat(const tf2::Quaternion& quat)
{
    checkReinit();
    if (m_quatInitialized)
    {
        m_quatAccu = m_quatAccu.slerp(quat, m_alpha);
    }
    else
    {
        m_quatAccu        = quat;
        m_quatInitialized = true;
    }

    m_timeOfLastValue = ros::Time::now();
}

void ExplonentialMovingAverageFilter::addPose(const Pose& pose)
{
    addVec3(pose.pos);
    addQuat(pose.rot);
}

void ExplonentialMovingAverageFilter::reset()
{
    m_scalarInitialized = false;
    m_quatInitialized   = false;
    m_vecInitialized    = false;
}

double ExplonentialMovingAverageFilter::scalar() const
{
    return m_scalarAccu;
}

tf2::Vector3 ExplonentialMovingAverageFilter::vec3() const
{
    return m_vectorAccu;
}

tf2::Quaternion ExplonentialMovingAverageFilter::quat() const
{
    return m_quatAccu;
}

Pose ExplonentialMovingAverageFilter::pose() const
{
    return Pose(vec3(), quat());
}

ros::Time ExplonentialMovingAverageFilter::timeOfLastValue() const
{
    return m_timeOfLastValue;
}

void ExplonentialMovingAverageFilter::checkReinit()
{
    if (m_timeout.toSec() == 0.0)
        return;

    if (ros::Time::now() - m_timeOfLastValue >= m_timeout)
        reset();
}

void ExplonentialMovingAverageFilter::setAlpha(double alpha)
{
    m_alpha = alpha;
}

double ExplonentialMovingAverageFilter::alpha() const
{
    return m_alpha;
}

void ExplonentialMovingAverageFilter::setTimeout(const ros::Duration& timeout)
{
    m_timeout = timeout;
}
