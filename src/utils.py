import textwrap


def camel_to_snake(s):
  return ''.join(['_' + c.lower() if c.isupper() else c for c in s]).lstrip('_')


def snake_to_camel(s):
  return ''.join(
      ['{:s}{:s}'.format(x[0].upper(), x[1:]) if len(x) > 1 else x for x in s.split('_')])


def indent(s, level=1):
  return textwrap.indent(s, '  ' * level)
