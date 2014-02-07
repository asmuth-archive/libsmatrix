# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name          = "libsmatrix"
  s.version       = "0.0.1"
  s.date          = Date.today.to_s
  s.platform      = Gem::Platform::RUBY
  s.authors       = ["Paul Asmuth", "Amir Friedman"]
  s.email         = ["paul@paulasmuth.com", "amirf@null.co.il"]
  s.homepage      = "http://github.com/paulasmuth/libsmatrix"
  s.summary       = %q{A thread-safe two dimensional sparse matrix data structure with C, Java and Ruby bindings.}
  s.description   = %q{A thread-safe two dimensional sparse matrix data structure with C, Java and Ruby bindings. It was created to make loading and accessing medium sized (10GB+) matrices in boxed languages like Java/Scala or Ruby easier.}
  s.licenses      = ["MIT"]
  s.extensions    = ['src/ruby/extconf.rb']
  s.files         = `git ls-files`.split("\n") - [".gitignore", ".rspec", ".travis.yml"]
  s.test_files    = `git ls-files -- spec/*`.split("\n")
  s.require_paths = ["src/ruby/"]

  s.add_development_dependency "rspec", "~> 2.8.0"
end
