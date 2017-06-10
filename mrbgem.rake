MRuby::Gem::Specification.new('mruby-cairo') do |spec|
  spec.license = 'BSD'
  spec.authors = 'Hiroki Mori'
  spec.linker.libraries << ['cairo', 'pixman-1', 'z']
#  spec.cc.include_paths << "/usr/local/include/cairo/"
#  spec.linker.library_paths << "/usr/local/lib"
  spec.cc.include_paths << "/storage/home/hiroki/obj/storage/home/hiroki/zrouter/ports/storage/home/hiroki/zrouter/ports/graphics/cairo/cairo-1.14.6/src"
  spec.linker.library_paths << "/storage/home/hiroki/obj/storage/home/hiroki/zrouter/Onion_Omega_rootfs/lib"
end
