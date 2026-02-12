class KeywordTest < Picotest::Test
  def test_keyword_can_be_method_name
    script = <<~RUBY
      def next(v)
        puts v
      end
      self.next("OK")
    RUBY
    actual = run_script(script)
    assert_equal('OK', actual)
  end
end
