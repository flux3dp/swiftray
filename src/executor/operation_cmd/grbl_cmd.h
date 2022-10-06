#ifndef GRBL_COMMAND_H
#define GRBL_COMMAND_H

#include "operation_cmd.h"
#include <periph/motion_controller/motion_controller.h>
#include <memory>


class GrblPauseCmd : public OperationCmd 
{
public:
  explicit GrblPauseCmd(QPointer<MotionController> motion_controller);
  ExecStatus execute(QPointer<Executor> executor) override;

private:
  QPointer<MotionController> motion_controller_;
};


class GrblResumeCmd : public OperationCmd 
{
public:
  explicit GrblResumeCmd(QPointer<MotionController> motion_controller);
  ExecStatus execute(QPointer<Executor> executor) override;

private:
  QPointer<MotionController> motion_controller_;
};


class GrblResetCmd : public OperationCmd 
{
public:
  explicit GrblResetCmd(QPointer<MotionController> motion_controller);
  ExecStatus execute(QPointer<Executor> executor) override;

private:
  QPointer<MotionController> motion_controller_;
};

class GrblStatusReportCmd : public OperationCmd
{
 public:
  explicit GrblStatusReportCmd(QPointer<MotionController> motion_controller);
  ExecStatus execute(QPointer<Executor> executor) override;

private:
  QPointer<MotionController> motion_controller_; 
};

class GrblUnlockCmd : public OperationCmd 
{
public:
  explicit GrblUnlockCmd(QPointer<MotionController> motion_controller);
  ExecStatus execute(QPointer<Executor> executor) override;

private:
  QPointer<MotionController> motion_controller_;
};

class GrblHomeCmd : public OperationCmd 
{
public:
  explicit GrblHomeCmd(QPointer<MotionController> motion_controller);
  ExecStatus execute(QPointer<Executor> executor) override;

private:
  QPointer<MotionController> motion_controller_;
};

class GrblBuildInfoCmd : public OperationCmd 
{
public:
  explicit GrblBuildInfoCmd(QPointer<MotionController> motion_controller);
  ExecStatus execute(QPointer<Executor> executor) override;

private:
  QPointer<MotionController> motion_controller_;
};

class GrblSettingsCmd : public OperationCmd 
{
public:
  explicit GrblSettingsCmd(QPointer<MotionController> motion_controller);
  ExecStatus execute(QPointer<Executor> executor) override;

private:
  QPointer<MotionController> motion_controller_;
};

class GrblGCodeParamCmd : public OperationCmd 
{
public:
  explicit GrblGCodeParamCmd(QPointer<MotionController> motion_controller);
  ExecStatus execute(QPointer<Executor> executor) override;

private:
  QPointer<MotionController> motion_controller_;
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

  static std::shared_ptr<OperationCmd> createGrblCmd(CmdType type, QPointer<MotionController> motion_controller) {
    switch (type) {
      case CmdType::kCtrlResume:
        return std::make_shared<GrblResumeCmd>(motion_controller);
      case CmdType::kCtrlPause:
        return std::make_shared<GrblPauseCmd>(motion_controller);
      case CmdType::kCtrlReset:
        return std::make_shared<GrblResetCmd>(motion_controller);
      case CmdType::kCtrlStatusReport:
        return std::make_shared<GrblStatusReportCmd>(motion_controller);
      case CmdType::kSysUnlock:
        return std::make_shared<GrblUnlockCmd>(motion_controller);
      case CmdType::kSysHome:
        return std::make_shared<GrblHomeCmd>(motion_controller);
      case CmdType::kSysBuildInfo:
        return std::make_shared<GrblBuildInfoCmd>(motion_controller);
      case CmdType::kSysGrblSettings:
        return std::make_shared<GrblSettingsCmd>(motion_controller);
      case CmdType::kSysGCodeParserState:
        return std::make_shared<GrblGCodeParamCmd>(motion_controller);
      default:
        Q_ASSERT_X(false, "GrblCmdFactory", "Support for this cmd hasn't benn implemented yet!");
        break;
    }
    return std::shared_ptr<OperationCmd>{nullptr};
  }
};

#endif // GRBL_COMMAND_H
