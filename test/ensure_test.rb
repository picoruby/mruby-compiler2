class EnsureTest < Picotest::Test
  def test_ensure_arg
    script = <<~RUBY
      def m
        begin
          1
        ensure
          puts 2
        end
      end
      puts m
    RUBY
    actual = run_script(script)
    assert_equal("2\n1", actual)
  end

  def test_begin_rescue_else_ensure_return_value
    script = <<~RUBY
      r = begin
        1+1
      rescue
        2+2
      else
        3+3
      ensure
        4+4
      end
      p r
    RUBY
    actual = run_script(script)
    assert_equal("6", actual)
  end
end
