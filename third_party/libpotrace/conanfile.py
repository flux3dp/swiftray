from conan import ConanFile
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.files import copy, chdir
from conan.tools.microsoft import unix_path
import os

# Get the directory of the current script
script_dir = os.path.dirname(os.path.abspath(__file__))

class LibpotraceConan(ConanFile):
    name = "libpotrace"
    version = "1.16"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"
    exports_sources = "potrace-1.16/*"

    def layout(self):
        self.folders.source = "../potrace-1.16"
        self.folders.build = "build"

    def generate(self):
        tc = AutotoolsToolchain(self)
        tc.generate()

    def build(self):
        with chdir(self, self.folders.source):
            autotools = Autotools(self)
            if self.settings.os == "Windows":
                # Use MSYS2 or Cygwin bash to run the configure script
                bash = unix_path(self, "C:\\msys64\\usr\\bin\\bash.exe")  # Adjust this path if necessary
                configure_script = unix_path(self, os.path.join(self.folders.source, "configure"))
                self.run(f'{bash} -c "{configure_script} --with-libpotrace CPPFLAGS= CXXFLAGS= CFLAGS="')
            else:
                autotools.configure(args=["--with-libpotrace"])
            
            autotools.make()

    def package(self):
        self.run("ls -al " + os.path.join(self.folders.source, "src", ".libs"))
        copy(self, "potracelib.h", src=os.path.join(self.folders.source, "src"), dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.la", src=os.path.join(self.folders.source, "src", ".libs"), dst=os.path.join(self.package_folder, "lib"))
        copy(self, "*.a", src=os.path.join(self.folders.source, "src", ".libs"), dst=os.path.join(self.package_folder, "lib"))
        copy(self, "*.dll", src=os.path.join(self.folders.source, "src", ".libs"), dst=os.path.join(self.package_folder, "bin"))
        # TODO: Handle Symlinks
        copy(self, "*.dylib", src=os.path.join(self.folders.source, "src", ".libs"), dst=os.path.join(self.package_folder, "lib"))

    def package_info(self):
        self.cpp_info.libs = ["libpotrace"]