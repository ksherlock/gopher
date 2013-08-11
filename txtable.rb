#!/usr/bin/env ruby -w


def dump_rules(rules)

    # create another hash for the hash code.
    
    index = []
    
    rules.each {|key, value|
    
        # 1.8 doesn't have getbyte()
        # string[] returns a byte in 1.8, string in 1.9
        byte = key.bytes.next()
        byte |= 0x20
        byte ^= key.length
        
        byte &= 0x0f
        
        index[byte] ||= []

        index[byte].push(key)
    }
    
    indent6 = "      "

    index.each_index {|ix|
    
        array = index[ix]
        
        next unless array
        
        printf("    case 0x%02x:\n", ix)
        
        array.each{|key|
        
            offset = 0
            printf("      // %s\n", key)
            printf("      if (size == %d\n", key.length)
            
            key.scan(/..?/) {|xx|
              tmp = xx.unpack("C*")
              tmp = tmp.map {|xxx| xxx | 0x20 }
              if tmp.length == 2
                tmp = (tmp[0]) + (tmp[1] << 8 )
                
                printf("        && (wp[%d] | 0x2020) == 0x%04x // '%s'\n",
                    offset, tmp, xx
                )
                offset += 1
              else
                tmp = tmp[0]
                printf("        && (cp[%d] | 0x20) == 0x%02x     // '%s'\n",
                    offset * 2, tmp, xx
                )
              end
            } # scan             
            
            puts("      ) {")
                    
            rules[key].each {|x|
                puts(indent6 +  x)    
            }
            puts("      }")
        
        }
        printf("      break;\n\n")
    }
    

end


ARGV.each {|filename|

    state = 0
    substate = 0
    
    header = []
    trailer = []
    tmp = []
    rule = nil
    
    rules = {}
    
    
    IO.foreach(filename) {|line|
    
        #line.chomp!
        line.sub!(/\s*$/, ''); #trim trailing space

        #next if line == ''
        
        if line == '%%'
            state = state + 1
            raise "Too many sections" if state > 3 
            next
        end

        case state
        when 0
            raise "invalid section" unless line == ''
            next
        when 1
            header.push(line)
            next
        
        when 2
            trailer.push(line)
            next
        end
    
        
        # state 3
        if !rule
            next if line == ''
            
            if line =~ /^'([a-zA-Z0-9.+_\/-]+)'\s*->$/
                rule = $1;
                raise "duplicate rule: #{rule}" if rules[rule] 
                next
            else
                raise "oops #{line}"
            end
        end
        
        if line == '.'
            rules[rule] = tmp
            tmp = []
            rule = nil
        else
            tmp.push(line)
        end
    }
    if state != 3 || rule 
        raise "unexpected EOF"
    end
    
    header.each {|x| puts x }
    
    dump_rules(rules)
    
    trailer.each{|x| puts x }
    
}

