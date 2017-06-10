##
## Cairo Test
##

assert("Cairo#hello") do
  t = Cairo.new "hello"
  assert_equal("hello", t.hello)
end

assert("Cairo#bye") do
  t = Cairo.new "hello"
  assert_equal("hello bye", t.bye)
end

assert("Cairo.hi") do
  assert_equal("hi!!", Cairo.hi)
end
