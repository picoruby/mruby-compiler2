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

  if mruby?
    def test_splat_error_class
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
      script = <<~RUBY
        x = raise rescue 99
        p x
      RUBY
      actual = run_script(script)
      assert_equal("99", actual)
    end

    def test_rescue_modifier_nested
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
      script = <<~RUBY
        result = (1 + 2) rescue 0
        p result
      RUBY
      actual = run_script(script)
      assert_equal("3", actual)
    end

    def test_rescue_modifier_returning_nil
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
      script = <<~RUBY
        def my_method
          raise rescue false
        end
        p my_method
      RUBY
      actual = run_script(script)
      assert_equal("false", actual)
    end
  end

end
