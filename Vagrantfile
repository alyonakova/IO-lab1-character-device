# -*- mode: ruby -*-
# vi: set ft=ruby :

VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|

    config.vm.box = "ubuntu/bionic64"

    config.vm.synced_folder ".", "/lab1"

    config.vm.provision 'upgrade', type: 'shell',
        inline: <<-SHELL
            apt-get update -qqy
            apt-get upgrade -qy
        SHELL

    config.vm.provision 'install-devtools', type: 'shell',
        inline: <<-SHELL
            apt-get install -y build-essential linux-headers-$(uname -r)
        SHELL

    config.vm.provision 'build-driver', type: 'shell',
        inline: <<-SHELL
            make -C /lab1
        SHELL
end
