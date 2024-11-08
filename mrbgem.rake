MRuby::Gem::Specification.new('mruby-compiler2') do |spec|
  spec.license = 'MIT'
  spec.author  = 'HASUMI Hitoshi'
  spec.summary = 'mruby compiler using a universal parser'

  lib_dir = "#{dir}/lib"
  cc.include_paths << "#{dir}/include"

  prism_dir = "#{lib_dir}/prism"
  ruby_dir = "#{lib_dir}/ruby"

  cc.defines.flatten!

  if cc.defines.any? { _1.match?(/\APICORUBY_NO_FLOAT(=|\z)/ || _1.match?(/\AMRB_NO_FLOAT(=|\z)/) }
    cc.defines << "MRC_NO_FLOAT"
  end
  if cc.defines.any? { _1.match?(/\APICORUBY_INT64(=|\z)/) || _1.match?(/\AMRB_INT64(=|\z)/) }
    cc.defines << "MRC_INT64"
  end
  if cc.defines.any? { _1.match?(/\APICORUBY_DEBUG(=|\z)/) || _1.match?(/\AMRB_DEBUG(=|\z)/) }

  if cc.defines.include? "MRC_CUSTOM_ALLOC"
    cc.defines << "PRISM_XALLOCATOR"
  end

  if cc.defines.include? "MRC_DEBUG"
    cc.defines << "MRC_DUMP_PRETTY"
  else
    cc.defines << "PRISM_BUILD_MINIMAL"
  end

  prism_templates_dir = "#{lib_dir}/prism/templates"
  cc.include_paths << "#{prism_dir}/include"

  task :deep_clean do
    rm_rf prism_dir
  end

  next if %w(clean deep_clean).include?(Rake.application.top_level_tasks.first)

  directory prism_dir do
    FileUtils.cd lib_dir do
      sh "git clone https://github.com/ruby/prism.git"
    end
    FileUtils.cd prism_dir do
      sh "git checkout v0.30.0"
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

