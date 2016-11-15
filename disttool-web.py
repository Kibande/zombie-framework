#!/usr/bin/env python

from __future__ import print_function
import json, os, shutil, subprocess, sys

def try_makedirs(path):
	try: os.makedirs(path)
	except: pass

if len(sys.argv) <= 2:
	print('usage: disttool-web.py <project-path> <emscripten-path>', file=sys.stderr)
	exit(-1)

basepath = os.path.join(os.getcwd(), sys.argv[1])
os.chdir(basepath)

emscripten_path = sys.argv[2]

# Load configuration
config = json.load(open('dist.json'))

project_name = config['project_name']
project_url = config['project_url']
project_assets_dir = 'assets'

build_dir = 'web_build'
products_dir = config['products_dir'] if 'products_dir' in config else 'dist'
dist_dir = config['dist_dir'] if 'dist_dir' in config else 'web-dist'

common_assets_path = config['common_assets_path']

products_dir = os.path.join(basepath, products_dir)

# Build project

try_makedirs(build_dir)
os.chdir(build_dir)

## Call emcmake to compile the project sources
product_js_filename = project_name + '.js'
product_js_path = os.path.join(products_dir, product_js_filename)

rc = subprocess.call([
	'python',
	os.path.join(basepath, emscripten_path, 'emcmake'),
	'cmake',
	#'-DCMAKE_TOOLCHAIN_FILE="\cmake\Modules\Platform\Emscripten.cmake"'
	'-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=' + products_dir,
	'-DCMAKE_BUILD_TYPE=RelMinSize',
	'-DBUILD_SHARED_LIBS=0',
	'-G',
	'MinGW Makefiles' if os.name == 'nt' else 'Unix Makefiles',
	'..'])

if rc != 0:
	sys.exit(-1)

rc = subprocess.call(['make'])

if rc != 0:
	sys.exit(-1)

os.chdir(basepath)

# Package distribution

try_makedirs(dist_dir)

if project_url:
	template_name = 'template_with_url.html'
else:
	template_name = 'template.html'

html = open(os.path.join(common_assets_path, template_name)).read().decode('utf-8')

html = html.replace('{{PROJECT_NAME}}', project_name)
html = html.replace('{{PROJECT_URL}}', project_url)

open(os.path.join(dist_dir, 'index.html'), 'wb').write(html.encode('utf-8'))

shutil.copyfile(product_js_path, os.path.join(dist_dir, project_name + '.js'))

# Build data package

## Get a list of all assets and build argument list to emscripten's file_packager.py
os.chdir(products_dir)
assets = os.listdir('.')

args = []

for path in assets:
	if path != product_js_filename:
		args += ['--embed', path]

## Execute file_packager.py
data_js_path = os.path.join(basepath, dist_dir, project_name + '-data.js')

subprocess.call([
	'python',
	os.path.join(basepath, emscripten_path, 'tools', 'file_packager.py'),
	'TARGET',
	'--js-output=' + data_js_path
	] + args)
