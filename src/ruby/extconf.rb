require "mkmf"

#have_header("ruby.h") or missing("ruby.h")
ruby_include = RbConfig::CONFIG["rubyhdrdir"] + "/ruby/"

mkmf_includes = <<EOF
RUBY_INCLUDE = #{ruby_include}
EOF

File.open(::File.expand_path("../Makefile.in", __FILE__), "w+") do |f|
  f.write(mkmf_includes)
end

$makefile_created = true
