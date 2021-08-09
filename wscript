import os
import time

project = 'shader.example'
top = '.'
out = 'build'

f_sh = 0

def options(ctx):
    ctx.load('compiler_c')
    ctx.load('compiler_cxx')

def configure(ctx):
    ctx.env.VERSION = os.popen('git rev-parse --short HEAD').read().rstrip()
    ctx.env.TIMESTAMP = str(int(time.time()))
    ctx.env.DEFINES = ['VERSION=0x'+ctx.env.VERSION, 'COMPILE_TIMESTAMP='+ctx.env.TIMESTAMP, 'DEBUG=0']
    print('→ configuring the project')
    ctx.find_program('strip', var='STRIP')
    ctx.find_program('gcc', var='CC')
    ctx.find_program('g++', var='CXX')
    ctx.env.CFLAGS = '-O2 -fPIC'.split()
    ctx.env.CFLAGS += '-g -ggdb -Wno-unused-variable -Wno-format -Wno-unused-but-set-variable -Wall'.split()
    ctx.env.CXXFLAGS = [] + ctx.env.CFLAGS
    ctx.env.CFLAGS += '-std=gnu99'.split()
    ctx.env.CXXFLAGS += '-std=gnu++14'.split()
    ctx.load('compiler_c')
    ctx.load('compiler_cxx')

def clean(ctx):
    print('Cleaned.')

def build(ctx):
    source_excl = ['']
    source_objlib = ctx.path.ant_glob('src/lib/*.cc', excl = source_excl)
    source_prog = ctx.path.ant_glob('src/*.cc', excl = source_excl)
    source_prog += ctx.path.ant_glob('src/*.c', excl = source_excl)
    includes = ['.', 'src']
    libs = ['GLEW', 'glfw', 'GL', 'glut', 'pthread']
    print(source_objlib)
    ctx.objects(target='objlib', includes = includes, source = source_objlib)
    ctx.program(target='shader.example', source = source_prog, use=['objlib'], libpath = ['/usr/local/lib'], lib=libs, includes = includes)
