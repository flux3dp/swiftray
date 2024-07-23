#ifndef BSL_COMMAND_H
#define BSL_COMMAND_H

#include "operation_cmd.h"
#include <periph/motion_controller/motion_controller.h>
#include <periph/motion_controller/bsl_motion_controller.h>
#include <memory>


class BSLPauseCmd : public OperationCmd 
{
public:
  explicit BSLPauseCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};


class BSLResumeCmd : public OperationCmd 
{
public:
  explicit BSLResumeCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};


class BSLResetCmd : public OperationCmd 
{
public:
  explicit BSLResetCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};

class BSLStatusReportCmd : public OperationCmd
{
 public:
  explicit BSLStatusReportCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override; 
};

class BSLUnlockCmd : public OperationCmd 
{
public:
  explicit BSLUnlockCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};

class BSLHomeCmd : public OperationCmd 
{
public:
  explicit BSLHomeCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};

class BSLBuildInfoCmd : public OperationCmd 
{
public:
  explicit BSLBuildInfoCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};

class BSLSettingsCmd : public OperationCmd 
{
public:
  explicit BSLSettingsCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};

class BSLGCodeParamCmd : public OperationCmd 
{
public:
  explicit BSLGCodeParamCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};



class BSLCmdFactory 
{
public:
  enum CmdType {
    kCtrlPause,           // !
    kCtrlResume,          // ~
    kCtrlReset,           // \x18
    kCtrlStatusReport,    // ?
    kSysUnlock,           // $X
    kSysHome,             // $H
    kSysBuildInfo,        // $I
    kSysBSLSettings,     // $$
    kSysGCodeParserState, // $G
    //kSysGCodeParam  // $#
    //kSysRestoreBSLSettings,
    //kSysRestoreGcodeParamSettings,
    //kSysRestoreAllSettings,
    //kSysSleep,
    //kSysCheck
  };

  static std::shared_ptr<OperationCmd> createBSLCmd(CmdType type) {
    switch (type) {
      case CmdType::kCtrlResume:
        return std::make_shared<BSLResumeCmd>();
      case CmdType::kCtrlPause:
        return std::make_shared<BSLPauseCmd>();
      case CmdType::kCtrlReset:
        return std::make_shared<BSLResetCmd>();
      case CmdType::kCtrlStatusReport:
        return std::make_shared<BSLStatusReportCmd>();
      case CmdType::kSysUnlock:
        return std::make_shared<BSLUnlockCmd>();
      case CmdType::kSysHome:
        return std::make_shared<BSLHomeCmd>();
      case CmdType::kSysBuildInfo:
        return std::make_shared<BSLBuildInfoCmd>();
      case CmdType::kSysBSLSettings:
        return std::make_shared<BSLSettingsCmd>();
      case CmdType::kSysGCodeParserState:
        return std::make_shared<BSLGCodeParamCmd>();
      default:
        Q_ASSERT_X(false, "BSLCmdFactory", "Support for this cmd hasn't benn implemented yet!");
        break;
    }
    return std::shared_ptr<OperationCmd>{nullptr};
  }
};

#endif // BSL_COMMAND_H
