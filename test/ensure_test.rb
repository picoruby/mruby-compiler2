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
end
