import argparse


def main():
  parser = argparse.ArgumentParser(description='Generate stuff sack Python library.')
  parser.add_argument('--lib', required=True, help='Stuff Sack shared library (*.so).')
  parser.add_argument('--spec', required=True, help='YAML message specification.')
  parser.add_argument('--output', required=True, help='Python library output.')
  args = parser.parse_args()

  library_str = \
f'''
from bazel_tools.tools.python.runfiles import runfiles

from src import py_stuff_sack

r = runfiles.Create()

globals().update(py_stuff_sack.get_globals(
    r.Rlocation('{args.lib}'),
    r.Rlocation('{args.spec}')
))
'''

  with open(args.output, 'w') as f:
    f.write(library_str)


if __name__ == '__main__':
  main()
