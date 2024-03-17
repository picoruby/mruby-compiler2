MRuby::Gem::Specification.new('mruby-compiler2') do |spec|
  spec.license = 'MIT'
  spec.author  = 'HASUMI Hitoshi'
  spec.summary = 'mruby compiler using a universal parser'

  lib_dir = "#{dir}/lib"
  prism_dir = "#{lib_dir}/prism"
  prism_templates_dir = "#{lib_dir}/prism/templates"
  static_prism = "#{prism_dir}/build/libprism.a"
  cc.include_paths << "#{dir}/include"
  cc.include_paths << "#{prism_dir}/include"

  task :deep_clean do
    rm_rf prism_dir
  end

  next if Rake.application.top_level_tasks.first == "deep_clean"

  directory prism_dir do
    FileUtils.cd lib_dir do
      sh "git clone https://github.com/ruby/prism.git"
    end
    FileUtils.cd prism_dir do
      sh "git checkout 532e4cc209818f52075bd4b2dc2f353d7da1bfbe"
    end
  end

  task :prism_templates => prism_dir do
    FileUtils.cd prism_dir do
      sh "templates/template.rb"
    end
  end

  TEMPLATE_GENERATES = %w(node prettyprint serialize token_type)

  TEMPLATE_GENERATES.each do |name|
    dst = "#{prism_dir}/src/#{name}.c"
    # file task does not work when dst does not exist. why?
    Rake::Task[:prism_templates].invoke unless File.exist?(dst)
    file dst => ["#{prism_templates_dir}/src/#{name}.c.erb", "#{prism_templates_dir}/template.rb"] do |t|
      Rake::Task[:prism_templates].invoke
    end
  end

  Dir.glob("#{prism_dir}/src/**/*.c").map do |src|
    obj = objfile(src.pathmap("#{build_dir}/lib/%n"))
    objs << obj
    file obj => [src] do |f|
      cc.run f.name, f.prerequisites.first
    end
  end

end

