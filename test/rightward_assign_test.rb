class RightwardAssignTest < Picotest::Test
  def test_rightward_assign
    script = <<~RUBY
      :hello.to_s => a
      puts a
    RUBY
    actual = run_script(script)
    assert_equal("hello", actual)
  end
end
