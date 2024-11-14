class EnsureTest < PicoRubyTest
  desc "ensure arg"
  assert_equal(<<~RUBY, "2\n1")
    def m
      begin
        1
      ensure
        puts 2
      end
    end
    puts m
  RUBY
end
