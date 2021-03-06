/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file 
 * distributed with this source distribution.
 * 
 * This file is part of REDHAWK core.
 * 
 * REDHAWK core is free software: you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as published by the 
 * Free Software Foundation, either version 3 of the License, or (at your 
 * option) any later version.
 * 
 * REDHAWK core is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */


#ifndef DEVICE_MANAGER_IMPL_H
#define DEVICE_MANAGER_IMPL_H

#include <string>
#include <map>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/shared_ptr.hpp>

#include <ossie/ComponentDescriptor.h>
#include <ossie/ossieSupport.h>
#include <ossie/DeviceManagerConfiguration.h>
#include <ossie/PropertySet_impl.h>
#include <ossie/PortSet_impl.h>
#include <ossie/FileManager_impl.h>
#include <ossie/Properties.h>
#include <ossie/SoftPkg.h>
#include <ossie/Events.h>
#include <ossie/affinity.h>
#include "spdSupport.h"
#include "process_utils.h"

#include <dirent.h>

class DeviceManager_impl: 
    public virtual POA_CF::DeviceManager,
    public PropertySet_impl,
    public PortSet_impl
{
    ENABLE_LOGGING

public:
  DeviceManager_impl (const char*, const char*, const char*, const char*, const struct utsname &uname, bool, const char *, bool *);

    ~DeviceManager_impl ();
    char* deviceConfigurationProfile ()
        throw (CORBA::SystemException);

    CF::FileSystem_ptr fileSys ()
        throw (CORBA::SystemException);

    char* identifier ()
        throw (CORBA::SystemException);

    char* label ()
        throw (CORBA::SystemException);

    CF::DomainManager_ptr domMgr ()
        throw (CORBA::SystemException);

    CF::DeviceSequence * registeredDevices ()
        throw (CORBA::SystemException);

    CF::DeviceManager::ServiceSequence * registeredServices ()
        throw (CORBA::SystemException);

    // Run this after the constructor
    void postConstructor( const char*) throw (CORBA::SystemException, std::runtime_error);

    void registerDevice (CF::Device_ptr registeringDevice)
        throw (CF::InvalidObjectReference, CORBA::SystemException);

    void unregisterDevice (CF::Device_ptr registeredDevice)
        throw (CF::InvalidObjectReference, CORBA::SystemException);

    void shutdown ()
        throw (CORBA::SystemException);
    void _local_shutdown ()
        throw (CORBA::SystemException);

    void registerService (CORBA::Object_ptr registeringService, const char* name)
        throw (CF::InvalidObjectReference, CORBA::SystemException);

    void unregisterService (CORBA::Object_ptr registeredService, const char* name)
        throw (CF::InvalidObjectReference, CORBA::SystemException);

    char* getComponentImplementationId (const char* componentInstantiationId)
        throw (CORBA::SystemException);

    bool getUseLogConfigResolver() { return _useLogConfigUriResolver; };

    void childExited (pid_t pid, int status);

    bool allChildrenExited ();

    bool isShutdown() { return  _adminState == DEVMGR_SHUTDOWN; };

    uint32_t  getClientWaitTime( ) { return CLIENT_WAIT_TIME; }

private:
    DeviceManager_impl ();   // No default constructor
    DeviceManager_impl(DeviceManager_impl&);  // No copying

    typedef   boost::shared_ptr<FileSystem_impl>                                  FileSystemPtr;
    typedef   std::vector<  ossie::ComponentPlacement >                           ComponentPlacements;
    typedef   std::pair< ossie::ComponentPlacement, local_spd::ProgramProfile* >  Deployment;
    typedef   std::vector< Deployment >                                           DeploymentList;
    typedef   std::list<std::pair<std::string,std::string> >                      ExecparamList;
    typedef   std::map<std::string, PackageMod  >                                 PackageMods;
    typedef   std::vector< PackageMod >                                           PackageModList;

    struct DeviceNode {
        std::string identifier;
        std::string label;
        std::string IOR;
        CF::Device_var device;
        pid_t pid;
    };

    struct ServiceNode{
        std::string identifier;
        std::string label;
        std::string IOR;
        CORBA::Object_var service;
        pid_t pid;
    };

    typedef std::vector<DeviceNode*> DeviceList;
    typedef std::vector<ServiceNode*> ServiceList;

    // Devices that are registered with this DeviceManager.
    DeviceList  _registeredDevices;
    ServiceList _registeredServices;

    // Devices that were launched by this DeviceManager, but are either waiting for
    // registration or process termination.
    DeviceList  _pendingDevices;
    ServiceList _pendingServices;

    // Properties
    std::string     logging_config_uri;
    StringProperty* logging_config_prop;
    std::string     HOSTNAME;
    float           DEVICE_FORCE_QUIT_TIME;
    CORBA::ULong    CLIENT_WAIT_TIME;

    // read only attributes
    struct utsname _uname;
    std::string processor_name;
    std::string os_name;
    std::string _identifier;
    std::string _label;
    std::string _deviceConfigurationProfile;
    std::string _fsroot;
    std::string _cacheroot;
    std::string _local_sdrroot;
    std::string _local_domroot;
    redhawk::affinity::CpuList cpu_blacklist;

    std::string _domainName;
    std::string _domainManagerName;
    CosNaming::Name_var base_context;
    CosNaming::NamingContext_var rootContext;
    CosNaming::NamingContext_var devMgrContext;
    CF::FileSystem_var _local_dom_filesys;
    CF::FileSystem_var _fileSys;
    CF::DeviceManager_var myObj;
    bool checkWriteAccess(std::string &path);

    enum DevMgrAdmnType {
        DEVMGR_REGISTERED,
        DEVMGR_UNREGISTERED,
        DEVMGR_SHUTTING_DOWN,
        DEVMGR_SHUTDOWN
    };
    
    DevMgrAdmnType        _adminState;
    CF::DomainManager_var _dmnMgr;

    void killPendingDevices(int signal, int timeout);
    void abort();

    void parseDeviceConfigurationProfile(const char *overrideDomainName);
    void parseSpd();
    void setupImplementationForHost();
    void getDomainManagerReferenceAndCheckExceptions();
    void resolveNamingContext();
    void bindNamingContext();

    const std::vector<const ossie::Property *> &getAllocationProperties();

    bool getCodeFilePath(
        std::string&                            codeFilePath,
        const local_spd::ImplementationInfo &   matchedDeviceImpl,
        ossie::SoftPkg&                         SPDParser,
        FileSystem_impl*&                       fs_servant,
	bool                                    useLocalFileSystem=true);

    void registerDeviceManagerWithDomainManager(
        CF::DeviceManager_var& my_object_var);

    void getCompositeDeviceIOR(
        std::string&                                  compositeDeviceIOR, 
        const std::vector<ossie::ComponentPlacement>& componentPlacements,
        const ossie::ComponentPlacement&              componentPlacementInst);

    bool addDeviceImplProperties (
        local_spd::ProgramProfile *compProfile,
        const local_spd::ImplementationInfo& deviceImpl );


    bool joinPRFProperties (
        const std::string& prfFile, 
        ossie::Properties& properties);
    
    CF::Properties getResourceOptions( const ossie::ComponentInstantiation& instantiation);


    int  resolveDebugLevel( const std::string &level_in );
    void resolveLoggingConfiguration( const std::string &                      usageName,
                                      std::vector< std::string >&              new_argv,
                                      const ossie::ComponentInstantiation&     instantiation,
                                      const std::string &logcfg_path );
    DeviceNode* getDeviceNode(const pid_t pid);

    bool getDeviceOrService(std::string& type, const local_spd::ProgramProfile *comp );

    void setEnvironment( ProcessEnvironment &env,
                         const std::vector< std::string > &deps,
                         const PackageMods  &pkgMods );

    void do_load( CF::FileSystem_ptr fs,
                  const std::string &fileName, 
                  const CF::LoadableDevice::LoadType &loadKind);

    void loadDependencies( local_spd::ProgramProfile *component,
                           CF::LoadableDevice_ptr  device,
                           const local_spd::SoftpkgInfoList & dependencies);

    void createDeviceCacheLocation(
        std::string&                         devcache,
        std::string&                         usageName, 
        const ossie::ComponentInstantiation& instantiation);

    void createDeviceExecStatement(
        std::vector< std::string >&                   new_argv,
        const ossie::ComponentPlacement&              componentPlacement,
	local_spd::ProgramProfile                     *compProfile,
        const std::string&                            componentType,
        const std::string&                            codeFilePath,
        const ossie::ComponentInstantiation&          instantiation,
        const std::string&                            usageName,
        const std::string&                            compositeDeviceIOR );

    void createDeviceThreadAndHandleExceptions(
        const ossie::ComponentPlacement&              componentPlacement,
	local_spd::ProgramProfile                     *compProfile,
        const std::string&                            componentType,
        const std::string&                            codeFilePath,
        const ossie::ComponentInstantiation&          instantiation,
        const std::string&                            compositeDeviceIOR );

    void createDeviceThread(
        const ossie::ComponentPlacement&              componentPlacement,
	local_spd::ProgramProfile                     *compProfile,
        const std::string&                            componentType,
        const std::string&                            codeFilePath,
        const ossie::ComponentInstantiation&          instantiation,
        const std::string&                            devcache,
        const std::string&                            usageName,
        const std::string&                            compositeDeviceIOR );

    ExecparamList createDeviceExecparams(
        const ossie::ComponentPlacement&              componentPlacement,
	local_spd::ProgramProfile                     *compProfile,
        const std::string&                            componentType,
        const std::string&                            codeFilePath,
        const ossie::ComponentInstantiation&          instantiation,
        const std::string&                            usageName,
        const std::string&                            compositeDeviceIOR );

    void recordComponentInstantiationId(
        const ossie::ComponentInstantiation& instantiation,
        const std::string &impl_id );

    local_spd::ProgramProfile *findProfile( const std::string &instantiationId );
    bool deviceIsRegistered (CF::Device_ptr);
    bool serviceIsRegistered (const char*);
    void getDomainManagerReference(const std::string&);
    void registerRogueDevice (CF::Device_ptr registeringDevice);

    // this mutex is used for synchronizing _registeredDevices, _pendingDevices, and _registeredServices
    boost::recursive_mutex registeredDevicesmutex;  
    boost::condition_variable_any pendingDevicesEmpty;
    void increment_registeredDevices(CF::Device_ptr registeringDevice);
    void increment_registeredServices(CORBA::Object_ptr registeringService, 
                                      const char* name);
    bool decrement_registeredDevices(CF::Device_ptr registeredDevice);
    bool decrement_registeredServices(CORBA::Object_ptr registeringService, 
                                      const char* name);
    void clean_registeredDevices();
    void clean_registeredServices();
    void clean_externalServices();

    void local_unregisterService(CORBA::Object_ptr service, const std::string& name);
    void local_unregisterDevice(CF::Device_ptr device, const std::string& name);

    bool resolveImplementation( local_spd::ProgramProfile* rsc );
    bool resolveSoftpkgDependencies( local_spd::ImplementationInfo* implementation );
    bool resolveSoftpkgDependencies( local_spd::ImplementationInfo* implementation, 
                                     const ossie::Properties&       host_props );

    local_spd::ImplementationInfo *resolveDependencyImplementation( const local_spd::SoftpkgInfo&        implementation, 
                                                                    const ossie::Properties&        host_props );
                        
    void deleteFileSystems();
    bool makeDirectory(std::string path);
    std::string getIORfromID(const char* instanceid);
    std::string deviceMgrIOR;
    std::string fileSysIOR;
    bool *_internalShutdown;
    bool _useLogConfigUriResolver;   
    bool skip_fork;

    // this mutex is used for synchronizing access to implementations and deployed components
    boost::mutex                       componentImplMapmutex;  
    std::map<std::string, std::string> _componentImplMap;
    DeploymentList                     deployed_comps;
    PackageMods                        sharedPkgs;

    
    // DeviceManager context... 
    ossie::DeviceManagerConfiguration          node_dcd;
    ossie::Properties                          host_props;
    local_spd::ProgramProfile                  *devmgr_info;

    // Registration record for Domain's IDM_Channel 
    ossie::events::EventChannelReg_var   idm_registration;
    std::string                          IDM_IOR;

};

#endif                                            /* __DEVICEMANAGER_IMPL__ */

