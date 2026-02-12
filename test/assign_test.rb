class AssignTest < Picotest::Test
  def test_simple_assign
    script = <<~RUBY
      a = 1
      puts a
    RUBY
    actual = run_script(script)
    assert_equal("1", actual)
  end

  def test_chained_assign_b
    script = <<~RUBY
      a = b = "hello"
      p b
    RUBY
    actual = run_script(script)
    assert_equal('"hello"', actual)
  end

  def test_chained_assign_a
    script = <<~RUBY
      a = b = "hello"
      p a
    RUBY
    actual = run_script(script)
    assert_equal('"hello"', actual)
  end

  def test_assign_in_block
    script = <<~RUBY
      a = 1
      1.times do
        a += 3
        p a
        b = a = 2
        p a, b
      end
      p a
    RUBY
    actual = run_script(script)
    assert_equal("4\n2\n2\n2", actual)
  end

  def test_attr_setter
    script = <<~RUBY
      class A
        attr_accessor :b
      end
      a = A.new
      a.b = 1
      puts a.b
    RUBY
    actual = run_script(script)
    assert_equal("1", actual)
  end

  def test_array_element_assign
    script = <<~RUBY
      a = [0, 1]
      a[0] = 2
      p a
    RUBY
    actual = run_script(script)
    assert_equal("[2, 1]", actual)
  end
end
