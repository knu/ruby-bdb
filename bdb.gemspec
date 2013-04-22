# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
#require 'bdb/version'

Gem::Specification.new do |spec|
  spec.name          = "bdb"
  spec.version       = "0.6.7"
  spec.authors       = ["Akinori MUSHA", "Guy Decoux"]
  spec.email         = ["knu@idaemons.org", nil]
  spec.description   = %q{This is a Ruby interface to Berkeley DB >= 2.0.}
  spec.summary       = %q{A Ruby interface to Berkeley DB >= 2.0}
  spec.homepage      = "https://github.com/knu/ruby-bdb"
  spec.license       = "Ruby's"

  spec.files         = `git ls-files`.split($/)
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.test_files    = spec.files.grep(%r{^tests/})
  spec.require_paths = ["lib"]

  spec.add_development_dependency "bundler", "~> 1.3"
  spec.add_development_dependency "rake"
  spec.add_development_dependency "rake-compiler", ">= 0.7.9"
end
