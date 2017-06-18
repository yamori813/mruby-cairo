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
c = Cairo.new(120, 160)
c.set_source_rgb(0, 0, 1)
c.move_to(0, 0)
c.line_to(100, 100)
c.stroke();
c.save_png("test.png");
```

You must install pixman and cairo library.

This is only prototype implementation.

## License
under the BSD License:
- see LICENSE file
