import os
import string
import sys

(
	script,
	output_cpp_path,
	output_h_path,
	output_dep_path,
	input_path,
	symbol_prefix,
) = sys.argv

script_path = os.path.realpath(__file__)
avatars_folder_path = os.path.dirname(input_path)

def load_contributors():
	contributors = []
	with open(input_path) as input_path_f:
		while contributor := input_path_f.readline():
			if '"' in contributor or "/" in contributor:
				raise Exception(f"Invalid data in {input_path}")
			contributors.append(contributor.strip())

	return contributors

def process_all(contributors):
	with open(output_cpp_path, 'w') as output_cpp_f, open(output_h_path, 'w') as output_h_f:
		output_cpp_f.write(f'''#include "{output_h_path}"
''')
		output_h_f.write(f'''#pragma once

#include <map>
#include <string>
#include "ResourceData.h"

extern std::map<std::string, ResourceData> {symbol_prefix}AvatarLookup;
''')

		map_lines = []
		for contributor in contributors:
			process_contributor(contributor, output_cpp_f, output_h_f, map_lines)

		output_cpp_f.write(f'''
std::map<std::string, ResourceData> {symbol_prefix}AvatarLookup = {{
''')
		for map_line in map_lines:
			output_cpp_f.write(map_line)
			output_cpp_f.write("\n")
		output_cpp_f.write("};")

	with open(output_dep_path, 'w') as output_dep_f:
		output_dep_f.write(f'''
{dep_escape(output_cpp_path)} {dep_escape(output_h_path)}: {dep_escape(input_path)} {dep_escape(script_path)}
''')

def process_contributor(contributor, output_cpp_f, output_h_f, map_lines):
	with open(f"{avatars_folder_path}/{contributor}.png", 'rb') as input_f:
		data = input_f.read()
	data_size = len(data)
	bytes_str = ', '.join([ str(ch) for ch in data ])

	escaped_contributor = username_escape(contributor)
	symbol_name = f"{symbol_prefix}_{escaped_contributor}_png"

	output_cpp_f.write(f'''
const unsigned char {symbol_name}[] = {{ {bytes_str} }};
const unsigned int {symbol_name}_size = {data_size};
''')

	output_h_f.write(f'''
extern const unsigned char {symbol_name}[];
extern const unsigned int {symbol_name}_size;
''')

	map_lines.append(f'\t{{ "{contributor}", {{ &{symbol_name}[0], {symbol_name}_size }} }},')

def dep_escape(s):
	t = ''
	for c in s:
		if c in [ ' ', '\\', ':', '$' ]:
			t += '\\'
		t += c
	return t

letters = []
def username_escape(s):
	ret = ''
	allow_numeric = False
	for c in s.lower():
		if c in string.ascii_lowercase:
			ret = ret + c
			allow_numeric = True
		if allow_numeric and c in string.digits:
			ret = ret + c
	return ret


contributors = load_contributors()
process_all(contributors)
