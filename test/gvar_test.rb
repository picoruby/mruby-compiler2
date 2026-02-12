class GvarTest < Picotest::Test
  def test_assign
    script = <<~RUBY
      $gvar = [0]
      p $gvar
    RUBY
    actual = run_script(script)
    assert_equal('[0]', actual)
  end

  def test_init_idiom
    script = <<~RUBY
      $gvar ||= "init"
      p $gvar
    RUBY
    actual = run_script(script)
    assert_equal('"init"', actual)
  end

  def test_op_assign
    script = <<~RUBY
      $gvar = 1
      $gvar += 1
      p $gvar
    RUBY
    actual = run_script(script)
    assert_equal('2', actual)
  end

  def test_op_assign_array
    script = <<~RUBY
      $gvar||=[]
      p $gvar
    RUBY
    actual = run_script(script)
    assert_equal('[]', actual)
  end

  def test_gvar_op_assign_hash
    script = <<~RUBY
      $gvar||={}
      p $gvar
    RUBY
    actual = run_script(script)
    assert_equal('{}', actual)
  end

  def test_op_assign_hash_2
    script = <<~RUBY
      $gvar={}
      $gvar[:a] ||= 1
      p $gvar
    RUBY
    actual = run_script(script)
    assert_equal('{a: 1}', actual)
  end
end
