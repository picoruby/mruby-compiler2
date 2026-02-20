class ForwardingTest < Picotest::Test
  def test_forwarding_only
    script = <<~RUBY
      def a0(*a, &b)
        p a
      end
      def a(...)
        a0(...)
      end
      a(1, 2, 3)
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2, 3]", actual)
  end

  def test_forwarding_with_leading_arg
    script = <<~RUBY
      def a0(*a, &b)
        p a
      end
      def b(x, ...)
        a0(x, ...)
      end
      b(1, 2, 3)
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2, 3]", actual)
  end
end
