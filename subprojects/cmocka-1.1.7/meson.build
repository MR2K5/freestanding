# Copyright © 2018 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

project(
  'cmocka',
  'c',
  version: '1.1.7',
  license: 'APLv2',
  meson_version: '>= 0.54.1',
  default_options: ['c_std=c99', 'buildtype=debugoptimized'],
)

lib_version = '0.8.0'

inc_include = include_directories('include')

#####################
# Config Generation #
#####################

cc_dict = {
  'compiler': meson.get_compiler('c'),
  'machine': host_machine,
  'native': false,
}

cc_native_dict = {
  'compiler': meson.get_compiler('c', native: true),
  'machine': build_machine,
  'native': true,
}

config_input_file = files('config.h.cmake')

subdir('config_native')
subdir('config_target')

###########################
# Subdirectory Processing #
###########################

libcmocka = library(
  'cmocka',
  'src/cmocka.c',
  c_args: '-DHAVE_CONFIG_H=1',
  include_directories: [
    inc_include,
    include_directories('config_target')
  ],
  vs_module_defs: 'src/cmocka.def',
  soversion: host_machine.system() != 'windows' ? lib_version.split('.')[0] : '',
  version: lib_version,
  dependencies: threads_dep,
  install: (meson.is_subproject() == false),
  build_by_default: (meson.is_subproject() == false),
)

libcmocka_native = library(
  'cmocka_native',
  'src/cmocka.c',
  c_args: '-DHAVE_CONFIG_H=1',
  include_directories: [
    inc_include,
    include_directories('config_native')
  ],
  vs_module_defs: 'src/cmocka.def',
  soversion: build_machine.system() != 'windows' ? lib_version.split('.')[0] : '',
  version: lib_version,
  dependencies: threads_dep,
  install: false, # Cannot install native targets in cross builds
  native: true,
  build_by_default: false,
)

######################
# Dependency Targets #
######################

# TODO: doc, include, tests, example
# Since we're using this as a wrap, and it's a unit test framework we're not
# going to install it.

cmocka_dep = declare_dependency(
  link_with: libcmocka,
  include_directories: inc_include,
  version: meson.project_version(),
)

cmocka_native_dep = declare_dependency(
  link_with: libcmocka_native,
  include_directories: inc_include,
  version: meson.project_version(),
)
