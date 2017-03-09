import os
import platform

env = DefaultEnvironment()
debug = ARGUMENTS.get('debug', 0)
release = ARGUMENTS.get('release', 0)

if int(release) == 1:
  env['objroot'] = env.Dir('release').abspath
  env['DEBUG'] = False
  src_env.Append(CCFLAGS=' -g -O -DNDEBUG')
else:
  env['objroot'] = env.Dir('debug').abspath
  env['DEBUG'] = True
  env.Append(CCFLAGS=' -g -Wall -Werror')

env['srcroot'] = env.Dir('.').abspath
objroot = env['objroot']
srcroot = env['srcroot']
env.Append(CPPPATH=[env['objroot'], env['srcroot'],'/home/work/protobuf/include'])
env.Append(CXXFLAGS='-std=c++11')
Export('env')

deps_libs = ['pthread', 'protobuf', 'gtest','glog','pthread']
def CheckDependencies():
  global env
  conf = Configure(env)
  for lib in deps_libs:
    if not conf.CheckLib(lib):
      print 'Did not find lib' + lib + ' existing'
      Exit(1)
  env = conf.Finish()
CheckDependencies()


def ProtoEmitter(target, source, env):
  ret_source = []
  ret_target = []
  for s in source:
    s_abspath = s.abspath
    target_prefix = s_abspath[:s_abspath.find('.proto')]
    ret_target += [
      target_prefix + ".pb.cc",
      target_prefix + ".pb.h"
    ]
    output = s_abspath.replace(env.Dir('$objroot').abspath, env.Dir('$srcroot').abspath)
    ret_source += [output]
  return ret_target, ret_source

def ProtoGenerator(target, source, env, for_signature):
  action = "protoc " + "--proto_path=$srcroot " + \
    "--cpp_out=$objroot "
  for s in source:
    action += s.abspath + " "
  return action

proto_builder = Builder(generator = ProtoGenerator,
  emitter=ProtoEmitter)
env['BUILDERS']['ProtocolBuffer'] = proto_builder


subdir = ['minirpc', 'test']

for x in subdir:
  target_dir = '%s/%s' % (env['objroot'], x)
  SConscript('%s/%s/SConscript' % (env['srcroot'], x), variant_dir=target_dir, duplicate=0)
