class SourceLineTest < Picotest::Test
  def test_line_number
    script = <<~RUBY
      p __LINE__
    RUBY
    actual = run_script(script)
    assert_equal("1", actual)
  end

  def test_line_number_multiline
    script = <<~RUBY
      x = 1
      y = 2
      p __LINE__
    RUBY
    actual = run_script(script)
    assert_equal("3", actual)
  end
end
