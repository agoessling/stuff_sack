import argparse


def main():
  parser = argparse.ArgumentParser(description='Generate stuff sack Python library.')
  parser.add_argument('--lib', required=True, help='Stuff Sack shared library (*.so).')
  parser.add_argument('--spec', required=True, help='YAML message specification.')
  parser.add_argument('--output', required=True, help='Python library output.')
  args = parser.parse_args()

  library_str = \
'''from src import py_stuff_sack

globals().update(py_stuff_sack.get_globals(
    '{}',
    '{}'
))'''

  with open(args.output, 'w') as f:
    f.write(library_str.format(args.lib, args.spec))


if __name__ == '__main__':
  main()
