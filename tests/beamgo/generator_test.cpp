#include <QRectF>
#include <toolpath_exporter/generators/gcode-generator.h>
#include <beamgo/compiler.h>
#include <iostream>
#include <stdexcept>

int main() {
  MachineSettings::MachineParam machine;
  machine.width = machine.height = 90;
  GCodeGenerator generator(machine, false);
  // Same planar setup/layer/finish calls as ToolpathExporter, with BeamGo homing disabled.
  generator.disableRotary();
  generator.setRedLight(false);
  generator.turnOffLaser();
  generator.useAbsolutePositioning();
  generator.setWorkarea(QRectF(0, 0, 90, 90));
  generator.turnOnLaser();
  generator.setFrequency(10);
  generator.turnOnLaser();
  generator.moveTo(35, 35, 2000, 0, 0);
  generator.moveTo(55, 35, 100, 15, 0);
  generator.moveTo(55, 55, 100, 15, 0);
  generator.moveTo(35, 55, 100, 15, 0);
  generator.moveTo(35, 35, 100, 15, 0);
  generator.turnOffLaser();
  generator.finishProgramFlow();
  const auto code = generator.toString();
  if (code.find("M100") == std::string::npos) throw std::runtime_error("Generator no longer emits M100");
  const auto records = beamgo::compile(code);
  beamgo::Bytes motion;
  for (size_t i = 0; i < records.size(); i += 5) {
    if (records[i] == 1 || records[i] == 2) motion.insert(motion.end(), records.begin() + i, records.begin() + i + 5);
  }
  beamgo::Bytes expected;
  for (const auto& point : {beamgo::record(2, 25485, 25485), beamgo::record(1, 40049, 25485),
                           beamgo::record(1, 40049, 40049), beamgo::record(1, 25485, 40049),
                           beamgo::record(1, 25485, 25485)}) expected.insert(expected.end(), point.begin(), point.end());
  if (motion != expected) throw std::runtime_error("Shared generator produced incorrect BeamGo motion");
  std::cout << "PASS: shared GCodeGenerator -> BeamGo compiler, zero-power travel and modal square\n";
}
