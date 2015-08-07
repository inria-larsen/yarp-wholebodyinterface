/*
 * Copyright (C) 2013 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Andrea Del Prete
 * email: andrea.delprete@iit.it
 *
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

#ifndef WBACTUATORS_YARP_H
#define WBACTUATORS_YARP_H

#include <yarp/dev/ControlBoardInterfaces.h>
#include <yarp/dev/IVelocityControl2.h>
#include <yarp/os/RateThread.h>
#include <yarp/os/Semaphore.h>
#include <yarp/os/BufferedPort.h>
#include <iCub/ctrl/adaptWinPolyEstimator.h>
#include <iCub/ctrl/filters.h>
#include <iCub/iDynTree/TorqueEstimationTree.h>
#include <iCub/skinDynLib/skinContactList.h>
#include "yarpWholeBodyInterface/yarpWbiUtil.h"
#include <map>

namespace yarp {
    namespace os {
        class Property;
        class Value;
    }
}

namespace wbi {
    class iWholeBodyActuators;
    class IDList;
    class ID;
    class Error;
}

namespace yarpWbi
{
    /* Constants */
    extern const std::string YarpWholeBodyActuatorsPropertyInteractionMode;
    extern const std::string YarpWholeBodyActuatorsPropertyInteractionModeStiff;
    extern const std::string YarpWholeBodyActuatorsPropertyInteractionModeCompliant;

    /**
     * Helper structure for representing a controlled joint, containing
     * both the id used to represent it in the wbi, and the axes in the
     * corresponding yarp control board
     */
    struct yarpWBAControlledJoint
    {
        int wbi_id;
        int wbi_controlboard_id;
        int yarp_controlboard_axis;
    };

    typedef std::vector< std::vector<yarpWBAControlledJoint> > yarpWBAControlledJointsControlBoardList;

    /**
     * Helper class for efficiently storing information about joints controlled
     * by the yarpWholeBodyActuactors interfaces, divided for controlboards
     *
     */
    class yarpWholeBodyActuatorsControlledJoints
    {
    public:
        yarpWBAControlledJointsControlBoardList positionControlledJoints; //< List of joints controlled in position for each controlboard
        yarpWBAControlledJointsControlBoardList positionDirectedControlledJoints; //< List of joints controlled in direct position for each controlboard
        yarpWBAControlledJointsControlBoardList velocityControlledJoints; //< List of joints controlled in velocity for each controlboard
        yarpWBAControlledJointsControlBoardList torqueControlledJoints;
        yarpWBAControlledJointsControlBoardList pwmControlledJoints;

        bool reset(const int nrOfControlBoards);
    };

    /*
     * Class for communicating with motor control boards of robot supporting a yarp interface.
     */
    class yarpWholeBodyActuators : public wbi::iWholeBodyActuators
    {
    protected:
        // true after init has been called, false before
        bool                               initDone;
        // name used as root for the local ports
        std::string                        name;
        // name of the robot (used for robot yarp ports)
        std::string                        robot;
        // list of names of the joints controlled in the interface
        wbi::IDList                     jointIdList;
        // list of names of the control boards used in the interface
        std::vector<std::string>           controlBoardNames;
        // list of controlBoard/Axis pair for each joint
        std::vector< std::pair<int,int> >  controlBoardAxisList;
        // number of joints controlled by wbi for each body part (size as controlBoardNames)
        //std::vector< int >                 controlledJointsInControlBoard;
        // total number of axes in each controlBoard (size as controlBoardNames. the one retunrned by getAxes)
        std::vector< int >                 totalAxesInControlBoard;
        // total number of axes in each controlBoard controlled by the wbi
        std::vector< int >                 totalControlledAxesInControlBoard;
        // total number
        // structure containing information of joints controlled in each mode for each controlboard
        yarpWholeBodyActuatorsControlledJoints controlledJointsForControlBoard;

        // current control mode of each joint (size: jointIdList.size())
        std::vector<wbi::ControlMode>        currentCtrlModes;

        // Map containing parameters to be read at initialization time
        yarp::os::Property wbi_yarp_properties;

         // yarp drivers vector whose index is given by relative index of the bodyPartNames vector
        std::vector<yarp::dev::IPositionControl*>     ipos;
        std::vector<yarp::dev::IPositionDirect*>      ipositionDirect;
        std::vector<yarp::dev::ITorqueControl*>       itrq;
        std::vector<yarp::dev::IImpedanceControl*>    iimp;
        std::vector<yarp::dev::IControlMode2*>        icmd;
        std::vector<yarp::dev::IInteractionMode*>     iinteraction;
        std::vector<yarp::dev::IVelocityControl2*>    ivel;
        std::vector<yarp::dev::IOpenLoopControl*>     iopl;
        std::vector<yarp::dev::PolyDriver*>           dd;

        /**
         * Open the yarp PolyDriver relative to control board bodyPartNames[bodyPart].
         */
        bool openControlBoardDrivers(int bodyPart);

        /**
         * Method called to update the internal data structures,
         * at initialization or when a control board control mode changes
         */
        bool updateControlledJointsForEachControlBoard();

        /** Convert the control modes defined in yarp/dev/IControlMode.h into the one defined in wbi. */
        wbi::ControlMode yarpToWbiCtrlMode(int yarpCtrlMode);

        /** Set the reference speed for the position control of the specified joint(s). */
        virtual bool setReferenceSpeed(double *rspd, int joint=-1);

        /** Set the proportional, derivative and integrale gain for the current joint(s) controller.
         * If you want to leave some values unchanged simply pass NULL to the corresponding gain
         * @param pValue Value(s) of the proportional gain.
         * @param dValue Value(s) of the derivative gain.
         * @param iValue Value(s) of the integral gain.
         * @param joint Joint number, if negative, all joints are considered.
         * @return True if operation succeeded, false otherwise. */
        bool setPIDGains(const double *pValue, const double *dValue, const double *iValue, int joint = -1);

        /** Set the offset (feedforward term) for the current joint(s) controller.
         * @param value Value(s) of the parameter.
         * @param joint Joint number, if negative, all joints are considered.
         * @return True if operation succeeded, false otherwise. */
        bool setControlOffset(const double *value, int joint = -1);

        /**
         * Private, single joint only version of setControlMode
         */
        bool setControlModeSingleJoint(wbi::ControlMode controlMode, double *ref, int joint);


    public:
        /**
         * Constructor.
         * @param _name Name of this object, used as a stem for opening YARP ports.
         * @param _robot Name of the robot, prefix for its yarp ports
         * @param yarp_wbi_properties yarp::os::Property object used to configure the interface
         *
         * \todo document format of yarp_wbi_properties
         */
        yarpWholeBodyActuators(const char* _name,
                               const yarp::os::Property & yarp_wbi_properties=yarp::os::Property());


        virtual ~yarpWholeBodyActuators();
        virtual bool init();
        virtual bool close();

        /**
         * Set the properties of the yarpWbiActuactors interface
         * Note: this function must be called before init, otherwise it takes no effect
         * @param yarp_wbi_properties the properties of the yarpWholeBodyActuators object
         */
        virtual bool setYarpWbiProperties(const yarp::os::Property & yarp_wbi_properties);

        /**
         * Get the properties of the yarpWbiActuactors interface
         * @param yarp_wbi_properties the properties of the yarpWholeBodyActuators object
         */
        virtual bool getYarpWbiProperties(yarp::os::Property & yarp_wbi_properties);


        virtual bool removeActuator(const wbi::ID &j);
        virtual bool addActuator(const wbi::ID &j);
        virtual int addActuators(const wbi::IDList &j);
        inline virtual const wbi::IDList& getActuatorList(){ return jointIdList; }

        /**
         * Set the control mode of the specified joint(s).
         * @param controlMode Id of the control mode.
         * @param ref Reference value(s) for the controller.
         * @param joint Joint number, if negative, all joints are considered.
         * @return True if operation succeeded, false otherwise.
         */
        virtual bool setControlMode(wbi::ControlMode controlMode, double *ref=0, int joint=-1);

        /**
         * Set the reference value for the controller of the specified joint(s).
         * @param ref Reference value(s) for the controller.
         * @param joint Joint number, if negative, all joints are considered.
         * @return True if operation succeeded, false otherwise.
         */
        virtual bool setControlReference(double *ref, int joint=-1);

        /**
         * Set a parameter (e.g. a gain) of one or more joint controllers.
         * @param paramId Id of the parameter.
         * @param value Value(s) of the parameter.
         * @param joint Joint number, if negative, all joints are considered.
         * @return True if operation succeeded, false otherwise.
         */
        virtual bool setControlParam(wbi::ControlParam paramId, const void *value, int joint=-1);


        virtual bool setControlProperty(std::string key, std::string value, int joint = -1, ::wbi::Error *error = 0);
    };
}

#endif

