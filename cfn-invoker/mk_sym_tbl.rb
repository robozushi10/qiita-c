#!/usr/bin/ruby
#------------------------------------------
# readelf -s *.o から関数シンボルを抜き出す
#------------------------------------------
def help
    printf("%s [input_file]\n", $0)
end

Fn_tbl_file  = "cfn.tbl.h"
Ext_dec_file = "cfn.extern.h"

# 引数チェック
if ARGV.length() != 1
    help()
end

if File.exists?(Fn_tbl_file) == true then
    File.unlink(Fn_tbl_file)
end

if File.exists?(Ext_dec_file) == true then
    File.unlink(Ext_dec_file)
end


while filename = ARGV.shift
    cmd = sprintf("| readelf -s %s", filename)
    f   = open(cmd)
    while line = f.gets
        if((line =~ /\sFUNC\s/) && (line =~ /\sGLOBAL\s/))
            if(line =~ /\s*(\w*):(\w*)\s*(\w*)\s(\w*)\s*(\w*)\s*(\w*)\s*(\w*)\s*(\w*)\s*(\w*)\s*(\w*)/)
                if($10 != "main")
                    buf  = sprintf("{\"%s\", &%s},\n", $10, $10)
                    buf2 = sprintf("extern %s;\n", $10)
                    File.open(Fn_tbl_file, 'a+').write(buf)
                    File.open(Ext_dec_file, 'a+').write(buf2)
                end
            end
        end
    end
end

