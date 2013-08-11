#!/bin/env ruby -w 

require 'erb'
require 'optparse'

class Option

  attr_reader :name
  attr_reader :description
  attr_reader :argument # nil, :, =, ?


  # name of the field, in the struct
  # will be null if virtual.
  attr_reader :field

  # array of custom code.
  attr_accessor :code

  attr_reader :modifiers


  def _fieldName(name, modifiers)
    return nil if modifiers[:virtual]
    return modifiers[:field] if modifiers[:field]
    return "_#{name}"
  end

  def initialize(name, argument, modifiers)
    @code = []

    modifiers ||= {}
    @name = name
    @field = _fieldName(name, modifiers)
    @argument = argument
    @modifiers = modifiers
  end

  def to_s()
    return "Option: #{description}#{argument} #{field}"
  end

  # def addCode(x)
  #   @code.push(x)
  # end

  # def finishCode
  #   # remove all blank lines at the end
  #   while @code.length && @code.last =~ /^\s*$/
  #     @code.pop
  #   end
  # end
  
end

class ShortOption < Option
  def initialize(name, argument, modifiers)
    super(name, argument, modifiers)

    @description = "-#{name}"
  end


  def generateCase()

    rv = []
    indent = " " * 8
    rv.push indent, "case '#{@name}':\n"
    indent += "  "

    if (@argument)
      #print indent, "GETOPTARG(#{@description})\n"
      rv.push indent, "++j;\n"
      rv.push indent, "if (cp[j]) {\n"
      rv.push indent, "  optarg = cp + j;\n"
      rv.push indent, "} else {\n"
      rv.push indent, "  ++i;\n"
      rv.push indent, "  if (i < argc) optarg = argv[i];\n"
      rv.push indent, "}\n"
      rv.push indent, "if (!optarg) {\n"
      rv.push indent, "  fputs(\"#{@description} requires an argument\\n\", stderr);\n"
      rv.push indent, "  exit(1);\n"
      rv.push indent, "}\n"
    end

    if @field
      if @argument
        rv.push indent, "options->#{@field} = optarg;\n"
      else
        if @modifiers[:increment]
          rv.push indent, "options->#{@field} += 1;\n"
        else
          rv.push indent, "options->#{@field} = 1;\n"
        end
      end
    end

    @code.each {|x| rv.push indent, x }
    rv.push indent,"break;\n"

  end

  def generateField()
    # generate the field for the struct.

    return "" unless @field
    return "  char *#{@field};\n" if @argument
    return "  unsigned #{@field};\n" if @modifiers[:increment]
    return "  unsigned #{@field}:1;\n"
  end
end


def arrayTrim(x)
  while x.length && x.last =~ /^\s*$/
    x.pop
  end
  x
end

def parseModifiers(string)

  # splits a key=value (, key=value)* string
  # key is equivalent to key={true}
  rv = {}
  return rv unless string
  args = string.split(',')
  args.each{|x|
    x.strip!
    next unless x.length

    key, value = x.split('=')

    value ||= true # key == key={true}
    key = key.intern

    rv[key] = value
  }

  return rv;
end


@shortRE = /
  ^
  -([\w])           # 1. letter
  ([:])?            # 2. argument type?
  \s*
  (?:\[
    ([\w\s,=]*)     # 3. modifiers?
  \])?
  \s*
  (->)?             # 4. code?
  $
  /x

@longRE = /
  ^
  --([\w-]*)        # 1. name
  ([=?])?           # 2. argument type?
  \s*
  (?:\[
    ([\w\s,=]*)     # 3. modifiers?
  \])?
  \s*
  (->)?             # 4. code?
  $
  /x

@headerTemplate = <<EOD
#ifndef __<%= config[:prefix] %>Options__
#define __<%= config[:prefix] %>Options__

typedef struct <%= config[:prefix] %>Options
{
<%= (config[:extra_fields] || []).join() %>
<%= (options.map {|x| x.generateField }).join() %>
} <%= config[:prefix] %>Options;

int Get<%= config[:prefix] %>Options(int argc, char **argv,
  <%= config[:prefix] %>Options *options);
#endif
EOD
#


def stripExtension(path)
  return $1 if path =~ /^(.*?)\.([^.\/]*)$/
  return path
end

def process(infile, keepName)

  opt = nil       # current option being processed
  tmp = []        # code block array
  callback = nil  # callback when code is finished.


  options = [] # list of options
  config = {} 

  infile.each_line { |line|

    line.chomp!

    line.rstrip!

    if callback
      if line == "" || line =~ /^\s+/
        tmp.push(line + "\n")
        next
      end
      # store...

      callback.call(arrayTrim(tmp));
      callback = nil
      tmp = []
      code = false
    end

    next if line =~ /^\s*#/

    if line =~ /^%/

      if m = line.match(/^%(\w+)=(\w+)$/)
        key = m[1].intern
        config[key] = m[2]
        next
      end

      if m = line.match(/^%(\w+)$/)
        key = m[1].intern
        config[key] = true
        next
      end

      if m = line.match(/^%(\w+)\s*->$/)
        key = m[1].intern
        callback = Proc.new {|code| config[key] = code }

        next
      end

      $stderr.puts "Not supported: #{line}" 
      next
    end

    if m = line.match(@shortRE)
      tmp = []
      flag = m[1]
      arg = m[2]
      modifiers = m[3]
      modifiers = parseModifiers(modifiers)
      opt = ShortOption.new(flag, arg, modifiers)
      options.push(opt)

      # any code?
      if m[4]
        callback = Proc.new {|code| opt.code = code }
      end

    end


    #
  }

  # if it ended within a code block...
  if callback
    callback.call(arrayTrim(tmp))
    callback = nil
    tmp = []
  end

  options.sort! {|a, b| a.name <=> b.name }


  keepName = nil if keepName == ""
  keepBase = stripExtension(keepName)

  headerName = (config[:prefix] || '') + 'Options.h'
  headerName = keepBase + '.h' if keepBase

  io = $stdout
  io = File.open(keepName, "w") if keepName

  b = binding
  erb = ERB.new(DATA.read(), 0, "%<>")

  io.write(erb.result(b))
  io.close unless io == $stdout


  # header file.
  io = $stdout
  io = File.open(headerName, "w") if headerName && keepName
  erb = ERB.new(@headerTemplate, 0, "%<>")
  io.write(erb.result(b))
  io.close unless io == $stdout

end

keepFile = nil

op = OptionParser.new {|opts|
  opts.banner = "Usage: options.rb [options] [infile]"

  opts.on("-o", "--keep OUTFILE", "Specify output file name") do |filename|
    keepFile = filename
  end
}

op.parse!


keepFile = nil if keepFile == ""

case ARGV.length
when 0
  process($stdin, keepFile)
when 1
  file = ARGV[0]
  io = $stdin
  if file != '-'
    keepFile = stripExtension(file) + '.c' if keepFile == nil
    io = File.open(file, "r")
  end
  process(io, keepFile)
  io.close unless io == $stdin
else
  op.help
  exit(1)
end



exit(0)

__END__
#ifdef __ORCAC__
#pragma optimize 79
#pragma noroot
#endif

#include <stdio.h>
#include <stdlib.h>

#include "<%= File.basename(headerName) %>"

<%= (config[:extra_includes] || []).join() %>

int Get<%= config[:prefix] %>Options(int argc, char **argv, 
  struct <%= config[:prefix] %>Options *options)
{
  int i, j;
  int eof = 0;
  int mindex = 1; /* mutation index */

  for (i = 1; i < argc; ++i)
  {
    char *cp = argv[i];
    char c = cp[0];

    <% if config[:posixly_correct] %>
    // stop processing at first non-opt.
    if (c != '-')
    {
      eof = 1;
      if (i == mindex) return argc;
      argv[mindex] = argv[i];
      ++mindex;
      continue;
    }
    <% end %>

    if (eof || c != '-')
    {
      if (mindex != i) argv[mindex] = argv[i];
      ++mindex;
      continue;
    }

    // long opt check would go here...
    if (cp[1] == '-' && cp[2] == 0)
    {
      eof = 1;
      continue;
    }

    // special case for '-'
    j = 0;
    if (cp[1] != 0) j = 1;

    for (; ; ++j)
    {
      char *optarg = 0;

      c = cp[j];
      if (!c) break;
      switch(c)
      {
<%= options.map{|o| o.generateCase() }.join() %>
        default:
          fprintf(stderr, "-%c : invalid option\n", c);
          exit(1);
          break;
      }
      // could optimize out if no options have flags.
      if (optarg) break;
    }
  }
  return mindex;
}
