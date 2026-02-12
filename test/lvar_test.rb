class LvarTest < Picotest::Test
  def test_implicit_initialize
    script = <<~RUBY
      p(a=a)
    RUBY
    actual = run_script(script)
    assert_equal('nil', actual)
  end
end
