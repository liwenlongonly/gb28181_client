<script setup>
import {
  Delete,
  VideoPlay,
  Search,
  EditPen
} from '@element-plus/icons-vue'

import {ref} from 'vue'

//设备列表数据模型
const deviceList = ref([])
const fileList = ref([])
//分页条数据模型
const pageNum = ref(1)//当前页
const totalCount = ref(20)//总条数
const pageSize = ref(10)//每页条数

//搜索条件
const searchDeviceId = ref('')
const searchDeviceStatus = ref(-1) // -1=全部, 0=关闭, 1=开启

//当每页条数发生了变化，调用此函数
const onSizeChange = (size) => {
  pageSize.value = size
  queryDeviceList()
}
//当前页码发生变化，调用此函数
const onCurrentChange = (num) => {
  pageNum.value = num
  queryDeviceList()
}

//回显文章分类
import {
  deviceCreateService,
  deviceStartService,
  deviceStopService,
  deviceDeleteService,
  deviceListService,
  deviceUpdateService,
  getVideoUrl
} from '@/api/device.js'

//获取设备列表数据
const queryDeviceList = async () => {
  let params = {
    pageNum: pageNum.value,
    pageSize: pageSize.value,
    deviceSipId: searchDeviceId.value || undefined,
    deviceStatus: searchDeviceStatus.value
  }
  let result = await deviceListService(params);
  //渲染视图
  totalCount.value = result.data.total;
  deviceList.value = result.data.items;
}

//搜索设备
const searchDevice = () => {
  pageNum.value = 1
  queryDeviceList()
}

//重置搜索
const resetSearch = () => {
  searchDeviceId.value = ''
  searchDeviceStatus.value = -1
  pageNum.value = 1
  queryDeviceList()
}

//状态筛选变化
const onStatusChange = () => {
  pageNum.value = 1
  queryDeviceList()
}

queryDeviceList();

import '@vueup/vue-quill/dist/vue-quill.snow.css'
import {Plus} from '@element-plus/icons-vue'
import {ElMessage} from 'element-plus'

//控制抽屉是否显示
const visibleDrawer = ref(false)
//抽屉模式: 'add' 添加, 'edit' 修改
const drawerMode = ref('add')
//添加表单数据模型
const defaultDeviceModel = {
  serverSipId: '37021200002000000122',
  serverPort: '5060',
  registerDomain: '3702120000',
  serverIp: '192.168.1.183',
  deviceSipId: '37021200001310000006',
  localPort: '5980',
  username: 'admin',
  password: 'admin',
  deviceName: 'IPC',
  filePath: ''
}
const deviceModel = ref({...defaultDeviceModel})

//上传成功的回调函数
const uploadSuccess = (result) => {
  deviceModel.value.filePath = result.data;
  console.log(result.data);
}

const uploadError = (result) => {
  ElMessage.error('文件上传失败');
  deviceModel.value.filePath = '';
}

const addDevice = async () => {
  if (deviceModel.value.filePath === '') {
    ElMessage.error("请先上传mp4视频文件");
    return;
  }
  //调用接口
  let result = await deviceCreateService(deviceModel.value);
  ElMessage.success(result.msg ? result.msg : '设备添加成功');
  closeDrawer();
  //刷新当前列表
  queryDeviceList()
}

// 打开添加设备抽屉
const openAddDrawer = () => {
  drawerMode.value = 'add'
  deviceModel.value = {...defaultDeviceModel}
  fileList.value = []
  visibleDrawer.value = true
}

// 打开编辑设备抽屉
const openEditDrawer = (row) => {
  drawerMode.value = 'edit'
  deviceModel.value = {
    serverSipId: row.server_sip_id,
    serverPort: String(row.server_port),
    registerDomain: row.register_domain || '',
    serverIp: row.server_ip,
    deviceSipId: row.device_sip_id,
    localPort: String(row.local_port),
    username: row.username || '',
    password: row.password || '',
    deviceName: row.device_name,
    filePath: row.file_path
  }
  fileList.value = []
  visibleDrawer.value = true
}

// 关闭抽屉并重置
const closeDrawer = () => {
  visibleDrawer.value = false
  deviceModel.value = {...defaultDeviceModel}
  fileList.value = []
}

// 提交（添加或修改）
const submitDevice = async () => {
  if (deviceModel.value.filePath === '') {
    ElMessage.error("请先上传mp4视频文件");
    return;
  }
  if (drawerMode.value === 'add') {
    await addDevice();
  } else {
    await updateDevice();
  }
}

// 修改设备
const updateDevice = async () => {
  if (deviceModel.value.filePath === '') {
    ElMessage.error("请先上传mp4视频文件");
    return;
  }
  let result = await deviceUpdateService(deviceModel.value);
  ElMessage.success(result.msg ? result.msg : '设备修改成功');
  closeDrawer();
  queryDeviceList();
}

const deleteDevice = async (row) => {
  if (!row.device_status) {
    let result = await deviceDeleteService(row.device_sip_id)
    ElMessage.success(`${row.device_sip_id}: 设备删除成功`);
    const index = deviceList.value.indexOf(row);
    if (index > -1) {
      deviceList.value.splice(index, 1);
    }
  }
}

const handleSwitchChange = async (row) => {
    if(row.device_status){
       let result = await deviceStartService(row.device_sip_id)
       ElMessage.success(`${row.device_sip_id}: 设备开启成功`);
    }else{
        let result = await deviceStopService(row.device_sip_id)
        ElMessage.success(`${row.device_sip_id}: 设备关闭成功`);
    }
}

// 视频播放相关
const videoDialogVisible = ref(false)
const videoUrl = ref('')
const videoTitle = ref('')
const videoPlayerRef = ref(null)

const playVideo = (row) => {
  videoTitle.value = `${row.device_name} (${row.device_sip_id})`
  videoUrl.value = getVideoUrl(row.device_sip_id)
  videoDialogVisible.value = true
}

const closeVideoDialog = () => {
  videoDialogVisible.value = false
  // 暂停视频释放资源
  if (videoPlayerRef.value) {
    videoPlayerRef.value.pause()
    videoPlayerRef.value.src = ''
  }
  videoUrl.value = ''
}

</script>
<template>
  <el-card class="page-container">
        <template #header>
            <div class="header">
                <span>设备管理</span>
                <div class="extra">
                    <el-input v-model="searchDeviceId" placeholder="输入设备ID搜索"
                              style="width: 240px; margin-right: 10px" clearable
                              @clear="searchDevice"
                              @keyup.enter="searchDevice">
                        <template #prefix>
                            <el-icon><Search /></el-icon>
                        </template>
                    </el-input>
                    <el-select v-model="searchDeviceStatus" placeholder="设备状态"
                               style="width: 130px; margin-right: 10px" @change="onStatusChange">
                        <el-option label="全部" :value="-1" />
                        <el-option label="已开启" :value="1" />
                        <el-option label="未开启" :value="0" />
                    </el-select>
                    <el-button type="primary" @click="searchDevice">搜索</el-button>
                    <el-button type="primary" @click="openAddDrawer" style="margin-left: 10px">添加设备</el-button>
                </div>
            </div>
        </template>

        <!-- 列表列表 -->
        <el-table :data="deviceList" style="width: 100%">
            <el-table-column label="设备ID" width="250" prop="device_sip_id"></el-table-column>
            <el-table-column label="名称" prop="device_name"></el-table-column>
            <el-table-column label="创建时间" width="200" prop="created_at"> </el-table-column>
            <el-table-column label="端口" width="80" prop="local_port"></el-table-column>
            <el-table-column label="文件路径" prop="file_path"></el-table-column>
            <el-table-column label="操作" width="250">
                <template #default="{ row }">
                    <el-switch v-model="row.device_status"
                               @change="handleSwitchChange(row)"></el-switch>
                    <el-button :icon="VideoPlay" circle plain type="success"
                               style="margin-left: 8px"
                               @click="playVideo(row)" title="播放视频"></el-button>
                    <el-button :disabled="row.device_status"
                               :icon="EditPen" circle plain type="warning"
                               @click="openEditDrawer(row)" title="修改设备"></el-button>
                    <el-button :disabled="row.device_status"
                               :icon="Delete" circle plain type="danger"
                               @click="deleteDevice(row)"></el-button>
                </template>

            </el-table-column>
            <template #empty>
                <el-empty description="没有数据" />
            </template>
        </el-table>
        <!-- 分页条 -->
        <el-pagination v-model:current-page="pageNum" v-model:page-size="pageSize" :page-sizes="[5, 10, 15, 20]"
            layout="jumper, total, sizes, prev, pager, next" background :total="totalCount" @size-change="onSizeChange"
            @current-change="onCurrentChange" style="margin-top: 20px; justify-content: flex-end" />
        <!-- 抽屉 -->
        <el-drawer v-model="visibleDrawer" :title="drawerMode === 'add' ? '添加设备' : '修改设备'"
                   direction="rtl" size="45%" @close="closeDrawer">
            <!-- 设备表单 -->
            <el-form :model="deviceModel" label-width="120px">
                <el-form-item label="SIP服务器编号:">
                    <el-input v-model="deviceModel.serverSipId" placeholder="请输入sip"></el-input>
                </el-form-item>
                <el-form-item label="SIP注册域:">
                    <el-input v-model="deviceModel.registerDomain" placeholder="请输入注册域"></el-input>
                </el-form-item>
                <el-form-item label="SIP服务器ip:">
                    <el-input v-model="deviceModel.serverIp" placeholder="请输入sip服务器ip"></el-input>
                </el-form-item>
                <el-form-item label="SIP服务器端口:">
                    <el-input v-model="deviceModel.serverPort" placeholder="请输入sip服务器端口"></el-input>
                </el-form-item>
                <el-form-item label="设备编号:">
                    <el-input v-model="deviceModel.deviceSipId" placeholder="请输入设备编号"
                              :disabled="drawerMode === 'edit'"></el-input>
                </el-form-item>
                <el-form-item label="设备本地端口:">
                    <el-input v-model="deviceModel.localPort" placeholder="请输入设备本地端口"></el-input>
                </el-form-item>
                <el-form-item label="设备名称:">
                    <el-input v-model="deviceModel.deviceName" placeholder="请输入设备名称"></el-input>
                </el-form-item>
                <el-form-item label="用户名:">
                    <el-input v-model="deviceModel.username" placeholder="请输入用户名"></el-input>
                </el-form-item>
                <el-form-item label="密码:">
                    <el-input v-model="deviceModel.password" placeholder="请输入密码"></el-input>
                </el-form-item>
                <el-form-item label="视频文件">
                    <div v-if="drawerMode === 'edit' && deviceModel.filePath" style="margin-bottom: 8px; color: #606266; font-size: 13px;">
                        当前文件: {{ deviceModel.filePath }}
                    </div>
                    <el-upload class="file-uploader"
                               :auto-upload="true"
                               :file-list="fileList"
                               :show-file-list="true"
                               action="/baseurl/file/upload"
                               accept=".mp4"
                               name="file"
                               :limit="1"
                               :on-success="uploadSuccess"
                               :on-error="uploadError">
                        <el-icon class="uploader-icon">
                            <Plus />
                        </el-icon>
                    </el-upload>
                </el-form-item>
                <el-form-item>
                    <el-button type="primary" @click="submitDevice">
                        {{ drawerMode === 'add' ? '添加' : '保存修改' }}
                    </el-button>
                    <el-button @click="closeDrawer">取消</el-button>
                </el-form-item>
            </el-form>
        </el-drawer>

        <!-- 视频播放弹窗 -->
        <el-dialog v-model="videoDialogVisible" :title="videoTitle" width="720px"
                   @close="closeVideoDialog" destroy-on-close>
            <video ref="videoPlayerRef" :src="videoUrl" controls
                   style="width: 100%; max-height: 480px; background: #000;">
                您的浏览器不支持视频播放
            </video>
        </el-dialog>
    </el-card>
</template>
<style lang="scss" scoped>
.page-container {
    min-height: 100%;
    box-sizing: border-box;

    .header {
        display: flex;
        align-items: center;
        justify-content: space-between;
    }
    .el-switch{
      width: 50px;
    }
}

/* 抽屉样式 */
.file-uploader {
    :deep() {
        .el-upload {
            border: 1px dashed var(--el-border-color);
            border-radius: 10px;
            cursor: pointer;
            position: relative;
            overflow: hidden;
            transition: var(--el-transition-duration-fast);
        }

        .el-upload:hover {
            border-color: var(--el-color-primary);
        }

        .el-icon.uploader-icon {
            font-size: 28px;
            color: #8c939d;
            width: 400px;
            height: 40px;
            text-align: center;
        }
    }
}
</style>
