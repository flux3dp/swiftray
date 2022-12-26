from conans import ConanFile, AutoToolsBuildEnvironment
from conans import tools

class libpotrace(ConanFile):
    name = "libpotrace"
    version = "1.16"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    src_dir = "potrace-1.16"
    src_src_dir = src_dir + "/src"
    exports_sources = src_dir + "/*"
    
    #def requirements(self):
    #    self.requires("zlib/1.2.13") # Failed to use for unknown reason, directly install pacman package (mingw-w64-x86_64-zlib) instead

    def build(self):
        with tools.chdir(self.src_dir):
            atools = AutoToolsBuildEnvironment(self, win_bash=tools.os_info.is_windows)
            print(atools.vars)
            if tools.os_info.is_windows:
                atools.configure(args=["--with-libpotrace", "CPPFLAGS=", "CXXFLAGS=", "CFLAGS="]) # use it to run "./configure" if using autotools
            else:
                atools.configure(args=["--with-libpotrace"])
            atools.make()

    def package(self):
        self.run("ls -al potrace-1.16/src")
        self.copy("potracelib.h", dst="include", src=self.src_src_dir)
        self.copy("*.la", dst="lib", src=(self.src_src_dir + "/.libs"))
        self.copy("*.a", dst="lib", src=(self.src_src_dir + "/.libs"))
        self.copy("*.dll", dst="bin", src=(self.src_src_dir + "/.libs"))
        # TODO: Handle Symlinks
        self.copy("*.dylib", dst="lib", src=(self.src_src_dir + "/.libs"))

    def package_info(self):
        self.cpp_info.libs = ["libpotrace"]
