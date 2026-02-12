class DstrTest < Picotest::Test
  def test_interpolation_1
    script = <<~'RUBY'
      @ivar = "Ruby"
      puts "hello #{@ivar}"
    RUBY
    actual = run_script(script)
    assert_equal('hello Ruby', actual)
  end

  def test_interpolation_2
    script = <<~'RUBY'
      puts "#{1+1}" + "a"
    RUBY
    actual = run_script(script)
    assert_equal('2a', actual)
  end

  def test_interpolation_3
    script = <<~'RUBY'
      i = 100
      puts "_" + "#{i+1}a" + "b"
    RUBY
    actual = run_script(script)
    assert_equal('_101ab', actual)
  end
end
