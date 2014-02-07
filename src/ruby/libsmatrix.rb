bundle_file = ::File.expand_path("../smatrix_ruby.bundle", __FILE__)
require bundle_file if ::File.exist? bundle_file

bundle_file = ::File.expand_path("../smatrix_ruby.so", __FILE__)
require bundle_file if ::File.exist? bundle_file

