require "mkmf"

mkmf_includes = <<EOF
RUBY_INCLUDE      = #{RbConfig::CONFIG["rubyhdrdir"]}
RUBY_INCLUDE_ARCH = #{RbConfig::CONFIG["rubyhdrdir"]}/#{RbConfig::CONFIG["arch"]}
RUBY_LIB          = #{RbConfig::CONFIG["libdir"]}
RUBY_SO_NAME      = #{RbConfig::CONFIG['RUBY_SO_NAME']}
LIBRUBYARG_SHARED = #{$LIBRUBYARG_SHARED}
LIBRUBYARG_STATIC = #{$LIBRUBYARG_STATIC}
LIBRUBYARG        = #{$LIBRUBYARG_SHARED.length > 0 ? $LIBRUBYARG_SHARED : $LIBRUBYARG_STATIC}
EOF

File.open(::File.expand_path("../Makefile.in", __FILE__), "w+") do |f|
  f.write(mkmf_includes)
end

$makefile_created = true

