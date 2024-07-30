#ifndef GRBL_COMMAND_H
#define GRBL_COMMAND_H

#include "operation_cmd.h"
#include <periph/motion_controller/motion_controller.h>
#include <memory>


class GrblPauseCmd : public OperationCmd 
{
public:
  explicit GrblPauseCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};


class GrblResumeCmd : public OperationCmd 
{
public:
  explicit GrblResumeCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};


class GrblResetCmd : public OperationCmd 
{
public:
  explicit GrblResetCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};

class GrblStatusReportCmd : public OperationCmd
{
 public:
  explicit GrblStatusReportCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};

class GrblUnlockCmd : public OperationCmd 
{
public:
  explicit GrblUnlockCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};

class GrblHomeCmd : public OperationCmd 
{
public:
  explicit GrblHomeCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};

class GrblBuildInfoCmd : public OperationCmd 
{
public:
  explicit GrblBuildInfoCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};

class GrblSettingsCmd : public OperationCmd 
{
public:
  explicit GrblSettingsCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};

class GrblGCodeParamCmd : public OperationCmd 
{
public:
  explicit GrblGCodeParamCmd();
  ExecStatus execute(QPointer<Executor> executor, QPointer<MotionController> motion_controller) override;
};



class GrblCmdFactory 
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
    kSysGrblSettings,     // $$
    kSysGCodeParserState, // $G
    //kSysGCodeParam  // $#
    //kSysRestoreGrblSettings,
    //kSysRestoreGcodeParamSettings,
    //kSysRestoreAllSettings,
    //kSysSleep,
    //kSysCheck
  };

  static std::shared_ptr<OperationCmd> createGrblCmd(CmdType type) {
    switch (type) {
      case CmdType::kCtrlResume:
        return std::make_shared<GrblResumeCmd>();
      case CmdType::kCtrlPause:
        return std::make_shared<GrblPauseCmd>();
      case CmdType::kCtrlReset:
        return std::make_shared<GrblResetCmd>();
      case CmdType::kCtrlStatusReport:
        return std::make_shared<GrblStatusReportCmd>();
      case CmdType::kSysUnlock:
        return std::make_shared<GrblUnlockCmd>();
      case CmdType::kSysHome:
        return std::make_shared<GrblHomeCmd>();
      case CmdType::kSysBuildInfo:
        return std::make_shared<GrblBuildInfoCmd>();
      case CmdType::kSysGrblSettings:
        return std::make_shared<GrblSettingsCmd>();
      case CmdType::kSysGCodeParserState:
        return std::make_shared<GrblGCodeParamCmd>();
      default:
        Q_ASSERT_X(false, "GrblCmdFactory", "Support for this cmd hasn't benn implemented yet!");
        break;
    }
    return std::shared_ptr<OperationCmd>{nullptr};
  }
};

#endif // GRBL_COMMAND_H
