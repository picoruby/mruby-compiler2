class RescueTest < Picotest::Test
  def test_simple_rescue
    script = <<~RUBY
      begin
        raise
      rescue
        p true
      end
    RUBY
    actual = run_script(script)
    assert_equal("true", actual)
  end

  def test_simple_else
    script = <<~RUBY
      begin
        0
      rescue
        p true
      else
        p false
      end
    RUBY
    actual = run_script(script)
    assert_equal("false", actual)
  end

  def test_simple_ensure
    script = <<~RUBY
      begin
        raise
      rescue
        p true
      else
        p false
      ensure
        p nil
      end
    RUBY
    actual = run_script(script)
    assert_equal("true\nnil", actual)
  end

  def test_rescue_error_class
    script = <<~RUBY
      begin
        raise "My Error"
      rescue => e
        puts e.class
        puts e.message
      end
    RUBY
    actual = run_script(script)
    assert_equal("RuntimeError\nMy Error", actual)
  end

  def test_retry
    script = <<~RUBY
      def my_method
        a = 0
        begin
          raise if a < 2
        rescue => e
          a += 1
          retry
        end
        p a
      end
      my_method
    RUBY
    actual = run_script(script)
    assert_equal("2", actual)
  end

  def test_splat_error_class
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      errors = [TypeError]
      begin
        raise TypeError
      rescue *errors
        puts "catched"
      end
    RUBY
    actual = run_script(script)
    assert_equal("catched", actual)
  end

  def test_splat_error_class_2
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      errors = [TypeError]
      begin
        raise TypeError
      rescue StandardError, *errors
        puts "catched"
      end
    RUBY
    actual = run_script(script)
    assert_equal("catched", actual)
  end

  def test_rescue_no_method_error
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      begin
        method_does_not_exist
      rescue ArgumentError => e
        puts "This should not happen 1"
      rescue NoMethodError => e
        puts "Yes"
      rescue => e
        puts "This should not happen 2"
      end
    RUBY
    actual = run_script(script)
    assert_equal("Yes", actual)
  end

  def test_rescue_modifier
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      def my_method
        raise rescue puts "rescued"
      end
      my_method
    RUBY
    actual = run_script(script)
    assert_equal("rescued", actual)
  end

  def test_rescue_modifier_with_return_value_no_error
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      def my_method
        42 rescue 0
      end
      p my_method
    RUBY
    actual = run_script(script)
    assert_equal("42", actual)
  end

  def test_rescue_modifier_with_return_value_with_error
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      def my_method
        raise rescue 100
      end
      p my_method
    RUBY
    actual = run_script(script)
    assert_equal("100", actual)
  end

  def test_rescue_modifier_with_assignment
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      x = raise rescue 99
      p x
    RUBY
    actual = run_script(script)
    assert_equal("99", actual)
  end

  def test_rescue_modifier_nested
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      def my_method
        raise rescue (raise rescue "nested")
      end
      p my_method
    RUBY
    actual = run_script(script)
    assert_equal("\"nested\"", actual)
  end

  def test_rescue_modifier_with_expression
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      result = (1 + 2) rescue 0
      p result
    RUBY
    actual = run_script(script)
    assert_equal("3", actual)
  end

  def test_rescue_modifier_returning_nil
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      def my_method
        raise rescue nil
      end
      p my_method
    RUBY
    actual = run_script(script)
    assert_equal("nil", actual)
  end

  def test_rescue_modifier_returning_false
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      def my_method
        raise rescue false
      end
      p my_method
    RUBY
    actual = run_script(script)
    assert_equal("false", actual)
  end

  def test_redo_inside_rescue_in_loop
    script = <<~RUBY
      loops = 0
      limit = 2
      loop do
        begin
          limit -= 1
          break unless limit > 0
          raise "!"
        rescue
          redo
        ensure
          loops += 1
        end
      end
      p loops
    RUBY
    actual = run_script(script)
    assert_equal("2", actual)
  end

end
