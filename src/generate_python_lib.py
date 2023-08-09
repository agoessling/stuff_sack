import argparse


def main():
  parser = argparse.ArgumentParser(description='Generate stuff sack Python library.')
  parser.add_argument('--lib', required=True, help='Stuff Sack shared library (*.so).')
  parser.add_argument('--spec', required=True, help='YAML message specification.')
  parser.add_argument('--output', required=True, help='Python library output.')
  args = parser.parse_args()

  library_str = \
f'''
import os

from src import py_stuff_sack

# The location of the relative location of the runfiles depends on whether the library is run as a
# tool or standalone.  We manually prepend the runfiles path to handle both cases.
workspace = os.path.basename(os.getcwd())

globals().update(py_stuff_sack.get_globals(
    os.path.join(os.environ['RUNFILES_DIR'], workspace, '{args.lib}'),
    os.path.join(os.environ['RUNFILES_DIR'], workspace, '{args.spec}')
))
'''

  with open(args.output, 'w') as f:
    f.write(library_str)


if __name__ == '__main__':
  main()
