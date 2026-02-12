class ScallTest < Picotest::Test
  def test_safe_navigation_operator
    script = <<~RUBY
      def a(v)
        case v
        when 0
          nil
        when 1
          true
        else
          false
        end
      end
      p a(0).to_i
      p a(0)&.to_i
      p a(2).to_s
      p a(2)&.to_s
    RUBY
    actual = run_script(script)
    assert_equal("0\nnil\n\"false\"\n\"false\"", actual)
  end

  def test_safe_navigation_operator_with_block
    script = <<~RUBY
      [0,1]&.each do |i|
        puts i
      end
    RUBY
    actual = run_script(script)
    assert_equal("0\n1", actual)
  end
end
