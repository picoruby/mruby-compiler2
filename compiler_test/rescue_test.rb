class RescueTest < PicoRubyTest

  desc "simple rescue"
  assert_equal(<<~RUBY, "true")
    begin
      raise
    rescue
      p true
    end
  RUBY

  desc "simple else"
  assert_equal(<<~RUBY, "false")
    begin
      0
    rescue
      p true
    else
      p false
    end
  RUBY

  desc "simple ensure"
  assert_equal(<<~RUBY, "true\nnil")
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

  desc "rescue error class"
  assert_equal(<<~RUBY, "RuntimeError\nMy Error")
    begin
      raise "My Error"
    rescue => e
      puts e.class
      puts e.message
    end
  RUBY

  desc "retry"
  assert_equal(<<~RUBY, "2")
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

  if @@vm_select == :mruby

    desc "splat error class"
    assert_equal(<<~RUBY, "catched")
      errors = [TypeError]
      begin
        raise TypeError
      rescue *errors
        puts "catched"
      end
    RUBY

    desc "splat error class 2"
    assert_equal(<<~RUBY, "catched")
      errors = [TypeError]
      begin
        raise TypeError
      rescue StandardError, *errors
        puts "catched"
      end
    RUBY

    desc "rescue NoMethodError"
    assert_equal(<<~RUBY, "Yes")
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

    desc "rescue modifier"
    assert_equal(<<~RUBY, "rescued")
      def my_method
        raise rescue puts "rescued"
      end
      my_method
    RUBY

    desc "rescue modifier with return value (no error)"
    assert_equal(<<~RUBY, "42")
      def my_method
        42 rescue 0
      end
      p my_method
    RUBY

    desc "rescue modifier with return value (with error)"
    assert_equal(<<~RUBY, "100")
      def my_method
        raise rescue 100
      end
      p my_method
    RUBY

    desc "rescue modifier with assignment"
    assert_equal(<<~RUBY, "99")
      x = raise rescue 99
      p x
    RUBY

    desc "rescue modifier nested"
    assert_equal(<<~RUBY, "\"nested\"")
      def my_method
        raise rescue (raise rescue "nested")
      end
      p my_method
    RUBY

    desc "rescue modifier with expression"
    assert_equal(<<~RUBY, "3")
      result = (1 + 2) rescue 0
      p result
    RUBY

    desc "rescue modifier returning nil"
    assert_equal(<<~RUBY, "nil")
      def my_method
        raise rescue nil
      end
      p my_method
    RUBY

    desc "rescue modifier returning false"
    assert_equal(<<~RUBY, "false")
      def my_method
        raise rescue false
      end
      p my_method
    RUBY

  end

end
