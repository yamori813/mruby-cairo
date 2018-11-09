MRuby::Gem::Specification.new('mruby-cairo') do |spec|
  spec.license = 'BSD'
  spec.authors = 'Hiroki Mori'
  spec.linker.libraries << ['cairo', 'pixman-1', 'png', 'z', 'freetype', 'fontconfig', 'expat']
  if(ENV['ZWORLDDESTDIR'] != nil)
    spec.cc.include_paths << (ENV['ZWORLDDESTDIR'] + '/usr/local/include/cairo/')
    spec.cc.include_paths << (ENV['ZWORLDDESTDIR'] + '/usr/local/include/freetype2')
    spec.cc.include_paths << (ENV['ZWORLDDESTDIR'] + '/usr/local/include')
    spec.linker.library_paths << (ENV['ZWORLDDESTDIR'] + '/usr/local/lib')
  else
    spec.cc.include_paths << "/usr/local/include/cairo/"
    spec.cc.include_paths << "/usr/local/include/freetype2"
    spec.cc.include_paths << "/usr/local/include/"
    spec.linker.library_paths << "/usr/local/lib"
  end

end
