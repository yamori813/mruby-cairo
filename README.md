# mruby-cairo   [![Build Status](https://travis-ci.org/yamori813/mruby-cairo.svg?branch=master)](https://travis-ci.org/yamori813/mruby-cairo)
Cairo class
## install by mrbgems
- add conf.gem line to `build_config.rb`

```ruby
MRuby::Build.new do |conf|

    # ... (snip) ...

    conf.gem :github => 'yamori813/mruby-cairo'
end
```
## example
```ruby
p Cairo.hi
#=> "hi!!"
t = Cairo.new "hello"
p t.hello
#=> "hello"
p t.bye
#=> "hello bye"
```

## License
under the BSD License:
- see LICENSE file
