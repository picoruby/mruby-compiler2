class IvarTest < Picotest::Test
  def test_assign
    script = <<~RUBY
      @ivar = [0]
      p @ivar
    RUBY
    actual = run_script(script)
    assert_equal('[0]', actual)
  end

  def test_init_idiom
    script = <<~RUBY
      @ivar ||= "init"
      p @ivar
    RUBY
    actual = run_script(script)
    assert_equal('"init"', actual)
  end

  def test_op_assign
    script = <<~RUBY
      @ivar = 1
      @ivar += 1
      p @ivar
    RUBY
    actual = run_script(script)
    assert_equal('2', actual)
  end

  def test_op_assign_array
    script = <<~RUBY
      @ivar||=[]
      p @ivar
    RUBY
    actual = run_script(script)
    assert_equal('[]', actual)
  end

  def test_op_assign_hash
    script = <<~RUBY
      @ivar||={}
      p @ivar
    RUBY
    actual = run_script(script)
    assert_equal('{}', actual)
  end

  def test_op_assign_hash_2
    script = <<~RUBY
      @ivar={}
      @ivar[:a] ||= 1
      p @ivar
    RUBY
    actual = run_script(script)
    assert_equal('{a: 1}', actual)
  end
end
