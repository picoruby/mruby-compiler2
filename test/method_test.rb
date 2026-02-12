class MethodTest < Picotest::Test
  def test_mysterious_case_different_from_p_space
    script = <<~RUBY
      p ()
    RUBY
    actual = run_script(script)
    assert_equal("nil", actual)
  end

  def test_another_space_case
    script = <<~RUBY
      p (1) do end
    RUBY
    actual = run_script(script)
    assert_equal("1", actual)
  end

  def test_integer_class
    script = <<~RUBY
      puts 1234.to_s
    RUBY
    actual = run_script(script)
    assert_equal("1234", actual)
  end

  def test_method_chain
    script = <<~RUBY
      puts 1234.to_s.to_i
    RUBY
    actual = run_script(script)
    assert_equal("1234", actual)
  end

  def test_scall
    script = <<~RUBY
      def my_block(&b)
        @cb = b
      end
      self.my_block do |v|
        p v
      end
      @cb&.call(1)
    RUBY
    actual = run_script(script)
    assert_equal("1", actual)
  end
end
