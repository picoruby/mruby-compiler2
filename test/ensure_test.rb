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

  def test_redo_in_rescue_triggers_ensure_in_for_loop
    script = <<~RUBY
      a = []
      count = 0
      for i in 0...1
        begin
          count += 1
          raise "err" if count < 3
          a << :done
        rescue
          a << :rescued
          redo
        ensure
          a << :ensure
        end
      end
      p a
    RUBY
    actual = run_script(script)
    assert_equal("[:rescued, :ensure, :rescued, :ensure, :done, :ensure]", actual)
  end

  def test_redo_in_rescue_triggers_ensure_in_while_loop
    script = <<~RUBY
      a = []
      count = 0
      while count < 1
        begin
          count += 1
          raise "err" if a.length < 2
          a << :done
        rescue
          a << :rescued
          redo
        ensure
          a << :ensure
        end
      end
      p a
    RUBY
    actual = run_script(script)
    assert_equal("[:rescued, :ensure, :done, :ensure]", actual)
  end

  def test_redo_complex_ensure_in_for_loop
    script = <<~RUBY
      def assert_equal(expected, actual)
        unless expected == actual
          raise "Expected \#{expected.inspect}, but got \#{actual.inspect}"
        end
      end
      a = []
      limit = 3
      e = RuntimeError.new("!")
      for i in 0...3
        begin
          limit -= 1
          break unless limit > 0
          a.push i * 3 + 1
          raise e
        rescue
          a.push i * 3 + 2
          redo
        ensure
          a.push i * 3 + 3
        end
      end
      assert_equal [1, 2, 3, 1, 2, 3, 3], a
      puts "ok"
    RUBY
    actual = run_script(script)
    assert_equal("ok", actual)
  end
end
