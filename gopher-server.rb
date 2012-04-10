#!/usr/bin/env ruby -w

require 'socket'
require 'optparse'



TEXT = {
    ".c" => true,
    ".h" => true,
    ".txt" => true,
    ".text" => true,
    ".rb" => true
}

def do_error(client, message)

    client.write("i#{message}\r\n")
    client.write(".\r\n")
    
end

def get_type(path)

    ext = File.extname(path).downcase
        
    return 0 if TEXT[ext]
    return 9
    
end

def dd(client, name, path, type)

  hostname = Thread.current[:hostname]
  port = Thread.current[:port]
  
  client.write("#{type}#{name}\t#{path}\t#{hostname}\t#{port}\r\n")
  
end

def do_directory(client, dir, root)

    dir.each do |file|
        next if file =~ /^\./
        type = nil
        
        path = dir.path + '/' + file
        st = File::Stat.new(path)
        if st.directory?
            dd(client, file + '/', root + file + '/', '1')      
            next
        end
        if st.file?
            type = get_type(file)
            dd(client, file, root + file, type)      
        end    
    end
    
    client.write(".\r\n")
end

def do_request(client)

    req = client.gets("\r\n").chomp()
    
    if req == nil || req == ""
        do_directory(client, Dir.new('.'), '/')
        return
    end
    
    # remove leading /
    req.sub!(%r{^/*},'')
    
    return do_error(client, 'Invalid request') if req =~ /\.\./
    
    begin
        st = File::Stat.new(req) or return do_error(client, 'Invalid Request')
    rescue SystemCallError
        return do_error(client, 'Invalid Request')
    end
    
    if st.directory?
        # check for /../
        req = '/' + req + '/'
        req.gsub!(%r{//}, "/")
        req.gsub!(%r{/./}, "/")
        
        do_error(client, "Invalid resource") if req =~ /\.\./
        
        do_directory(client, Dir.new('.' + req), req)
        return
    end
    
    return unless st.file?
    
    if get_type(req) == 0
        # text
        IO.foreach(req, "\n") {|x|
            x.chomp!
            x = "." + x if x =~ /^\./ 
            client.write(x)
            client.write("\r\n")
        }
        client.write(".\r\n")
    else
        # binary
        client.binmode()
        File.open(req, "rb") do |io|
        
            loop do
                x = io.read(256) or break
                client.write(x)
            end
        
        end
    end
end

# main



hostname = "imac.local"
port = 7070

OptionParser.new { |opts|

    opts.banner = "Usage: gopher-server [-p port]"
    
    opts.on('-p P', '--port P', Integer, 'Port') do |x|
        port = x
        puts "port = #{port}"
    end
    
    opts.on_tail('-h', '--help', 'Help') do
        puts opts
        exit
    end


}.parse!


server = TCPServer.new("0.0.0.0", port)
loop do
    Thread.start(server.accept) do |client|
        begin
            addr = client.getpeername() # client.remote_address()  # client.peeraddr(false)
            addr = addr.unpack("C*")
            peer = addr[4,4].join('.')
            
            Thread.current[:hostname] = hostname
            Thread.current[:port] = port
            
            puts("accept from #{addr.join(' ')}")
            
            do_request(client)
            client.flush()
            sleep(1) # for marinetti
        
        rescue Exception => error
            puts("Error: #{error}")
            print error.backtrace.join("\n")
        end
        
        client.close()
        puts("connection closed")
    end
end
    
