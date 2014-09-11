#!/usr/bin/env ruby -w

# process the flags.yaml file
# and generate a flags.h and flags.c file.
#

#
# todo -- support for long-options (--longoption, --longoption=value, etc)
#
#

require 'erb'
require 'yaml'

header_preamble = <<EOF

#ifndef __flags_h__
#define __flags_h__

typedef struct Flags {
EOF

header_postamble = <<EOF
} Flags;


extern struct Flags flags;

int FlagsParse(int argc, char **argv);

void FlagsHelp(void);

#endif

EOF


class Option
	@@map = {
		# some of these are a bad idea but whatever...
		'>' => 'gt',
		'<' => 'lt',
		',' => 'comma',
		'.' => 'period',
		'/' => 'forward_slash',
		'\\' =>  'back_slash',
		'?' => 'question',
		'|' => 'pipe',
		'~' => 'tilde',
		'`' => 'grave',
		'!' => 'bang',
		'@' => 'at',
		'#' => 'hash',
		'$' => 'dollar',
		'%' => 'percent',
		'^' => 'caret',
		'&' => 'ampersand',
		'*' => 'star',
		'(' => 'left_paren',
		')' => 'right_paren',
		'-' => 'minus',
		'+' => 'plus',
		'=' => 'equal',
		'[' => 'left_bracket',
		']' => 'right_bracket',
		'{' => 'left_brace',
		'}' => 'right_brace',
		':' => 'colon',
		';' => 'semi_colon',
		'\'' => 'apostrophe',
		'"' => 'quote'
	}

	def initialize(hash)

		@char = hash['char'].to_s
		@argument = hash['argument'] || false
		@increment = hash['increment'] || false

		@flag_name = hash['flag_name']
		@flag_name = @flag_name.to_s if @flag_name

		@xor = hash['xor'] || []
		@xor = case @xor
		when Array
			@xor
		when Integer, String
			[ @xor ]
		else
			raise "Invalid xor type: #{@xor}"
		end

		@xor.map! { |x| x.to_s }
	end

	attr_reader :char, :xor, :argument, :increment

	def flag_name
		return @flag_name if @flag_name
		return self.class.flag_name(@char)
	end

	def self.flag_name(char)
		return '_' + @@map[char] if @@map[char] 
		return '_' + char
	end


end

# better ARGF.
def argf_each 

	if ARGV.count > 0

		ARGV.each {|file|

			File.open(file, "r") {|io| 
				yield file, io 
			}
		}

	else
		yield nil, $stdin
	end

end


def escape_cstr(x)

	# escape a c string

	x.gsub(/([\\"])/, "\\\\1")
end


code = ERB.new(DATA.read(), 0, "%<>")

argf_each {|filename, file|


	data = YAML.load(file)

	help = data['help']
	options = data['options']

	# options is an array of items which may be hashes, strings, or numbers.
	# normalize them.

	options = options.map {|opt|

		opt = case opt
		when String, Integer
			{ 'char' => opt }
		when Hash
			# {'o' => { ... }}
			# or
			# {'char' => , ... }
			if opt['char']
				opt
			else
				opt = opt.first
				opt[1].merge({ 'char' => opt[0] })
			end
		else
			raise "Unexpected data type: #{opt}"
		end

		Option.new(opt)
	}

	#data[options] = options
	# check for help?

	basename = filename
	basename = $1 if filename && filename =~ /^(.*)\./

	b = binding # bind help, options for ERB.

	io = basename ? File.open(basename + ".c", "w") : $stdout
	io.write(code.result(b))
	
	io.close unless io == $stdout


	io = basename ? File.open(basename + ".h", "w") : $stdout
	io.write(header_preamble)
	# two passes - one with arguments, one without.
	options.each {|opt|
		if opt.argument
			io.printf("    char *%s;\n", opt.flag_name)
		elsif opt.increment
			io.printf("    unsigned %s;\n", opt.flag_name)
		end
	}
	io.puts()
	options.each {|opt|
		if !opt.argument && !opt.increment
			io.printf("    unsigned %s:1;\n", opt.flag_name)
		end
	}
	io.puts

	io.write(header_postamble)
	io.close unless io == $stdout


#	#puts options.to_yaml
#	puts code.result(binding())

}

__END__

#ifdef __ORCAC__
#pragma optimize 79
#pragma noroot
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "flags.h"

void FlagsHelp(void)
{
% help.each do |h|
  fputs("<%= escape_cstr(h) %>\n", stdout);
% end
  fputs("\n", stdout);
  exit(0);
}

int FlagsParse(int argc, char **argv)
{
  char *cp;
  char c;
  int i;
  int j;
    
  memset(&flags, 0, sizeof(flags));

  for (i = 1; i < argc; ++i)
  {    
    cp = argv[i];
    c = cp[0];
      
    if (c != '-')
      return i;
      
    // -- = end of options.
    if (cp[1] == '-' && cp[2] == 0)
      return i + 1;
   
    // now scan all the flags in the string...
    for (j = 1; ; ++j)
    {
      int skip = 0;
          
      c = cp[j];
      if (c == 0) break;
          
      switch (c)
      {
% if help && !options.find_index {|x| x.char == 'h' } 
      case 'h':
        FlagsHelp();
        break;
% end            
% #
% options.each do |opt|
      case '<%= escape_cstr(opt.char) %>':
% # check for an argument.
% flag_name = 'flags.' + opt.flag_name
% #
% if opt.argument
        // -xarg or -x arg
        skip = 1;
        if (cp[j + 1])
        {
          <%= flag_name %> = cp + j + 1;
        }
        else
        {
          if (++i >= argc)
          {
            fprintf(stderr, "option requires an argument -- %c\n", c); 
            return -1;
          }
          <%= flag_name %> = argv[i];
        }
% elsif opt.increment
        <%= flag_name %>++;
% else # no argument.
        <%= flag_name %> = 1;
% end # if no argument.
% #
% # unset any exclusive or values
% opt.xor.each do |xor_opt|
        flags.<%= Option.flag_name(xor_opt) %> = 0;
%end
        break;
% end # options.each

      default:
        fprintf(stderr, "illegal option -- %c\n", c);
        return -1; 
      }
            
      if (skip) break;
    }    
  }

  return i;
}
