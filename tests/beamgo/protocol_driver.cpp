#include <beamgo/protocol.h>
#include <beamgo/compiler.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

beamgo::Bytes unhex(const std::string& hex) {
  if (hex.size() % 2) throw std::invalid_argument("Odd hex length");
  beamgo::Bytes bytes;
  for (size_t i = 0; i < hex.size(); i += 2) {
    bytes.push_back(std::stoul(hex.substr(i, 2), nullptr, 16));
  }
  return bytes;
}

void print(const beamgo::Bytes& bytes) {
  for (auto byte : bytes) std::cout << std::hex << std::setfill('0') << std::setw(2) << unsigned(byte);
}

int main() {
  std::string line;
  while (std::getline(std::cin, line)) {
    std::istringstream input(line);
    std::string operation, hex;
    input >> operation >> hex;
    try {
      const auto bytes = unhex(hex);
      if (operation == "tx") {
        print(beamgo::encodeFrame(bytes));
      } else if (operation == "rx") {
        size_t split;
        input >> split;
        if (split > bytes.size()) throw std::invalid_argument("Split out of range");
        beamgo::FrameDecoder decoder;
        auto messages = decoder.push(beamgo::Bytes(bytes.begin(), bytes.begin() + split));
        auto remaining = decoder.push(beamgo::Bytes(bytes.begin() + split, bytes.end()));
        messages.insert(messages.end(), remaining.begin(), remaining.end());
        bool first = true;
        for (const auto& message : messages) {
          if (!first) std::cout << ',';
          print(message);
          first = false;
        }
      } else if (operation == "gcode" || operation == "framing") {
        beamgo::CompileOptions options;
        options.preview = operation == "framing";
        if (options.preview) input >> options.framing_power;
        print(beamgo::compile(std::string(bytes.begin(), bytes.end()), options));
      } else if (operation == "mark-blocks" || operation == "preview-blocks") {
        bool first = true;
        for (const auto& block : beamgo::jobBlocks(bytes, operation == "preview-blocks")) {
          if (!first) std::cout << ',';
          print(block);
          first = false;
        }
      } else {
        throw std::invalid_argument("Unknown operation");
      }
    } catch (const std::exception& error) {
      std::cout << "ERROR:" << error.what();
    }
    std::cout << std::endl;
  }
}
