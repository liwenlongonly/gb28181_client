<script setup>
import {
  Delete,
} from '@element-plus/icons-vue'

import {ref} from 'vue'

//设备列表数据模型
const deviceList = ref([])
const fileList = ref([])
//分页条数据模型
const pageNum = ref(1)//当前页
const totalCount = ref(20)//总条数
const pageSize = ref(5)//每页条数

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
  deviceListService
} from '@/api/device.js'

//获取设备列表数据
const queryDeviceList = async () => {
  let params = {
    pageNum: pageNum.value,
    pageSize: pageSize.value,
  }
  let result = await deviceListService(params);
  //渲染视图
  totalCount.value = result.data.total;
  deviceList.value = result.data.items;
}

queryDeviceList();

import '@vueup/vue-quill/dist/vue-quill.snow.css'
import {Plus} from '@element-plus/icons-vue'
import {ElMessage} from 'element-plus'

//控制抽屉是否显示
const visibleDrawer = ref(false)
//添加表单数据模型
const deviceModel = ref({
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
})

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
  //
  ElMessage.success(result.msg ? result.msg : '设备添加成功');
  //让抽屉消失
  visibleDrawer.value = false;
  deviceModel.value.filePath = '';
  fileList.value = [];
  //刷新当前列表
  queryDeviceList()
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

</script>
<template>
  <el-card class="page-container">
        <template #header>
            <div class="header">
                <span>设备管理</span>
                <div class="extra">
                    <el-button type="primary" @click="visibleDrawer = true">添加设备</el-button>
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
            <el-table-column label="操作" width="120">
                <template #default="{ row }">
                    <el-switch v-model="row.device_status"
                               @change="handleSwitchChange(row)"></el-switch>
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
        <el-drawer v-model="visibleDrawer" title="添加设备" direction="rtl" size="45%">
            <!-- 添加设备表单 -->
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
                    <el-input v-model="deviceModel.deviceSipId" placeholder="请输入设备编号"></el-input>
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
                    <!-- 
                        auto-upload:设置是否自动上传
                        action:设置服务器接口路径
                        name:设置上传的文件字段名
                        headers:设置上传的请求头
                        on-success:设置上传成功的回调函数
                     -->
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
                    <el-button type="primary" @click="addDevice()">添加</el-button>
                </el-form-item>
            </el-form>
        </el-drawer>
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