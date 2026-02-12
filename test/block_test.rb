class BlockTest < Picotest::Test
  def test_do_block
    script = <<~RUBY
      [0, 1].each do |i|
        puts i
      end
    RUBY
    actual = run_script(script)
    assert_equal("0\n1", actual)
  end

  def test_do_block_chain
    script = <<~RUBY
      a = [0, 1]
      a.each do |i|
        a[i] += 1
      end.each do |i|
        puts i
      end
    RUBY
    actual = run_script(script)
    assert_equal("1\n2", actual)
  end

  def test_brace_block
    script = <<~RUBY
      [0, 1].each { |i| puts i }
    RUBY
    actual = run_script(script)
    assert_equal("0\n1", actual)
  end

  def test_brace_block_chain
    script = <<~RUBY
      a = [0, 1]
      a.each { |i| a[i] += 1 }.each { |i| puts i }
    RUBY
    actual = run_script(script)
    assert_equal("1\n2", actual)
  end

  def test_hash_each_do_k_v
    script = <<~RUBY
      h = {a: 0, b: 1}
      h.each do |k, v|
        puts k
        puts v
      end
    RUBY
    actual = run_script(script)
    assert_equal("a\n0\nb\n1", actual)
  end

  def test_each_with_index
    script = <<~RUBY
      a = [true, false]
      a.each_with_index do |val, index|
        puts val
        puts index
      end
    RUBY
    actual = run_script(script)
    assert_equal("true\n0\nfalse\n1", actual)
  end

  def test_block_call
    script = <<~RUBY
      def my_method(&block)
        block.call
      end
      my_method {puts "hello"}
    RUBY
    actual = run_script(script)
    assert_equal("hello", actual)
  end

  def test_yield
    script = <<~RUBY
      def my_method
        yield
      end
      my_method {puts "hello"}
    RUBY
    actual = run_script(script)
    assert_equal("hello", actual)
  end

  def test_lvar_scope
    script = <<~RUBY
      a = 1
      [0].each do |a|
        puts a
      end
    RUBY
    actual = run_script(script)
    assert_equal("0", actual)
  end

  def test_return_blk
    script = <<~RUBY
      def a
        [1].each { return true }
      end
      p a
    RUBY
    actual = run_script(script)
    assert_equal("true", actual)
  end

  def test_interpolation_of_lvar_that_is_a_block_arg
    script = <<~'RUBY'
      [0].each { |i|
        puts "#{i + 2}"
      }
    RUBY
    actual = run_script(script)
    assert_equal("2", actual)
  end
end
