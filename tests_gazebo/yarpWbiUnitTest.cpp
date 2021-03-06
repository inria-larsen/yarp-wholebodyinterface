/*
 * Copyright (C) 2014 Open Source Robotics Foundation
 * Copyright (C) 2014 Fondazione Istituto Italiano di Tecnologia
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <gtest/gtest.h>

#include "GazeboYarpServerFixture.hh"

#include "gazebo_yarp_test_config.h"

#include <yarp/os/Time.h>

#include <wbi/wbiUtil.h>

#include "../include/yarpWholeBodyInterface/yarpWbiUtil.h"

#include "../include/yarpWholeBodyInterface/yarpWholeBodyInterface.h"

class yarpWbInterfaceUnitTest : public GazeboYarpServerFixture
{
};

/////////////////////////////////////////////////
/*
TEST_F(yarpWbiActuatorsUnitTest, basicGazeboYarpLoadingTest)
{
  Load("double_pendulum.world", true);


  gazebo::physics::WorldPtr world = gazebo::physics::get_world("default");
  ASSERT_TRUE(world != NULL);
}
*/

/////////////////////////////////////////////////
TEST_F(yarpWbInterfaceUnitTest, basicWbInterfaceLoadingTest)
{
  Load("double_pendulum.world", false);


  gazebo::physics::WorldPtr world = gazebo::physics::get_world("default");
  ASSERT_TRUE(world != NULL);


  bool is_yarp_network_active = yarp::os::NetworkBase::checkNetwork(1.0);
  ASSERT_TRUE(is_yarp_network_active);


  yarpWbi::yarpWholeBodyInterface    doublePendulumWbi("test_double_pendulum");

  yarp::os::Property wbiInterfaceProperties;
  wbiInterfaceProperties.fromConfigFile(std::string(YARP_CONF_PATH)+"/"+"wbi_double_pendulum.ini",true);
  wbiInterfaceProperties.put("urdf_file",std::string(YARP_CONF_PATH)+"/double_pendulum/model.urdf");

  //Test getting lists
  wbi::wbiIdList main_joints_lists;
  ASSERT_TRUE(yarpWbi::loadIdListFromConfig("DOUBLE_PENDULUM_JOINTS",wbiInterfaceProperties,main_joints_lists));

  ASSERT_FALSE(main_joints_lists.containsId(wbi::wbiId("medium_joint")));
  ASSERT_TRUE(main_joints_lists.containsId(wbi::wbiId("upper_joint")));
  ASSERT_TRUE(main_joints_lists.containsId(wbi::wbiId("lower_joint")));
  ASSERT_TRUE(main_joints_lists.size() == 2);


  doublePendulumWbi.setYarpWbiProperties(wbiInterfaceProperties);

  ASSERT_TRUE(doublePendulumWbi.addActuator(wbi::wbiId("upper_joint")));
  ASSERT_TRUE(doublePendulumWbi.addActuator(wbi::wbiId("lower_joint")));
  //ASSERT_FALSE(doublePendulumWbi.addActuator(wbi::wbiId("third_joint")));

  ASSERT_TRUE(doublePendulumWbi.addJoint(wbi::wbiId("upper_joint")));
  ASSERT_TRUE(doublePendulumWbi.addJoint(wbi::wbiId("lower_joint")));
  //ASSERT_FALSE(doublePendulumWbi.addSensor(wbi::SENSOR_ENCODER_POS,wbi::wbiId("third_joint")));

  //ASSERT_TRUE(doublePendulumWbi.addJoints(main_joints_lists));

  //std::cout << "doublePendulumActuactors.init()" << std::endl;
  ASSERT_TRUE(doublePendulumWbi.init());

  ASSERT_TRUE(doublePendulumWbi.getJointList().containsId(wbi::wbiId("upper_joint")));
  ASSERT_TRUE(doublePendulumWbi.getJointList().containsId(wbi::wbiId("lower_joint")));

  ASSERT_EQ(doublePendulumWbi.getActuatorList().size(),2);
  ASSERT_EQ(doublePendulumWbi.getJointList().size(),2);


  yarp::sig::Vector real_q(doublePendulumWbi.getDoFs(),-10);
  yarp::sig::Vector desired_q(doublePendulumWbi.getActuatorList().size());
  yarp::sig::Vector ref_dq(doublePendulumWbi.getActuatorList().size(),40.0*M_PI/180.0);


  ASSERT_TRUE(doublePendulumWbi.getEstimates(wbi::ESTIMATE_JOINT_POS,real_q.data(),0,true));

  std::cout << "Read position " << real_q[0] << " " << real_q[1] << std::endl;

  desired_q[0] = real_q[0] + M_PI/2;
  desired_q[1] = real_q[1] + M_PI/2;

  ASSERT_TRUE(doublePendulumWbi.setControlMode(wbi::CTRL_MODE_POS));
  ASSERT_TRUE(doublePendulumWbi.setControlParam(wbi::CTRL_PARAM_REF_VEL, ref_dq.data()));
  ASSERT_TRUE(doublePendulumWbi.setControlReference(desired_q.data()));

  //Wait to reach the desired position
  yarp::os::Time::delay(5.0);

  ASSERT_TRUE(doublePendulumWbi.getEstimates(wbi::ESTIMATE_JOINT_POS,real_q.data(),0,true));

  double tol = 0.1;

  EXPECT_NEAR(desired_q[0],real_q[0],tol);
  EXPECT_NEAR(desired_q[1],real_q[1],tol);

  desired_q[0] = real_q[0] - M_PI;
  desired_q[1] = real_q[1] - M_PI;

  ASSERT_TRUE(doublePendulumWbi.setControlMode(wbi::CTRL_MODE_POS));
  ASSERT_TRUE(doublePendulumWbi.setControlReference(desired_q.data()));

  //Wait to reach the desired position
  yarp::os::Time::delay(5.0);

  ASSERT_TRUE(doublePendulumWbi.getEstimates(wbi::ESTIMATE_JOINT_POS,real_q.data()));

  EXPECT_NEAR(desired_q[0],real_q[0],tol);
  EXPECT_NEAR(desired_q[1],real_q[1],tol);

  ASSERT_TRUE(doublePendulumWbi.close());
  ASSERT_TRUE(doublePendulumWbi.close());
}

/////////////////////////////////////////////////
/// Main
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
