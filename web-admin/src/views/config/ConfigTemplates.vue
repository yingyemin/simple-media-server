<template>
  <div class="config-templates">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>配置模板管理</span>
          <div class="header-actions">
            <el-button type="primary" @click="showCreateDialog">
              <el-icon><Plus /></el-icon>
              新建模板
            </el-button>
            <el-button @click="showImportDialog">
              <el-icon><Upload /></el-icon>
              导入模板
            </el-button>
            <el-button @click="exportAllTemplates">
              <el-icon><Download /></el-icon>
              导出全部
            </el-button>
          </div>
        </div>
      </template>

      <!-- 搜索和过滤 -->
      <div class="filter-section">
        <el-row :gutter="20">
          <el-col :span="8">
            <el-input
              v-model="searchText"
              placeholder="搜索模板名称或描述"
              clearable
              @input="handleFilter"
            >
              <template #prefix>
                <el-icon><Search /></el-icon>
              </template>
            </el-input>
          </el-col>
          <el-col :span="6">
            <el-select
              v-model="categoryFilter"
              placeholder="选择分类"
              clearable
              @change="handleFilter"
            >
              <el-option label="全部分类" value="" />
              <el-option label="RTMP" value="rtmp" />
              <el-option label="RTSP" value="rtsp" />
              <el-option label="WebRTC" value="webrtc" />
              <el-option label="SRT" value="srt" />
              <el-option label="其他" value="other" />
            </el-select>
          </el-col>
          <el-col :span="6">
            <el-select
              v-model="typeFilter"
              placeholder="模板类型"
              clearable
              @change="handleFilter"
            >
              <el-option label="全部类型" value="" />
              <el-option label="默认模板" value="default" />
              <el-option label="自定义模板" value="custom" />
            </el-select>
          </el-col>
          <el-col :span="4">
            <el-button @click="refreshTemplates" :loading="loading">
              <el-icon><Refresh /></el-icon>
              刷新
            </el-button>
          </el-col>
        </el-row>
      </div>

      <!-- 模板列表 -->
      <el-table
        :data="filteredTemplates"
        style="width: 100%"
        v-loading="loading"
        @selection-change="handleSelectionChange"
      >
        <el-table-column type="selection" width="55" />
        <el-table-column prop="name" label="模板名称" width="200">
          <template #default="scope">
            <div>
              <strong>{{ scope.row.name }}</strong>
              <el-tag v-if="scope.row.isDefault" type="success" size="small" style="margin-left: 8px;">
                默认
              </el-tag>
            </div>
          </template>
        </el-table-column>
        <el-table-column prop="description" label="描述" />
        <el-table-column prop="category" label="分类" width="100">
          <template #default="scope">
            <el-tag :type="getCategoryTagType(scope.row.category)">
              {{ scope.row.category.toUpperCase() }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="author" label="创建者" width="120" />
        <el-table-column prop="createTime" label="创建时间" width="180">
          <template #default="scope">
            {{ formatTime(scope.row.createTime) }}
          </template>
        </el-table-column>
        <el-table-column label="操作" width="200">
          <template #default="scope">
            <el-button type="text" @click="viewTemplate(scope.row)">查看</el-button>
            <el-button type="text" @click="editTemplate(scope.row)">编辑</el-button>
            <el-button type="text" @click="applyTemplate(scope.row)">应用</el-button>
            <el-button type="text" @click="exportTemplate(scope.row)">导出</el-button>
            <el-button
              type="text"
              style="color: #f56c6c;"
              @click="deleteTemplate(scope.row)"
              :disabled="scope.row.isDefault"
            >
              删除
            </el-button>
          </template>
        </el-table-column>
      </el-table>

      <!-- 分页 -->
      <div class="pagination-container">
        <el-pagination
          v-model:current-page="currentPage"
          v-model:page-size="pageSize"
          :page-sizes="[10, 20, 50, 100]"
          :total="totalTemplates"
          layout="total, sizes, prev, pager, next, jumper"
          @size-change="handleSizeChange"
          @current-change="handleCurrentChange"
        />
      </div>
    </el-card>

    <!-- 创建/编辑模板对话框 -->
    <el-dialog
      v-model="templateDialogVisible"
      :title="isEditing ? '编辑模板' : '创建模板'"
      width="800px"
      :before-close="handleTemplateDialogClose"
    >
      <el-form :model="templateForm" :rules="templateRules" ref="templateFormRef" label-width="100px">
        <el-form-item label="模板名称" prop="name">
          <el-input v-model="templateForm.name" placeholder="请输入模板名称" />
        </el-form-item>
        <el-form-item label="模板描述" prop="description">
          <el-input
            v-model="templateForm.description"
            type="textarea"
            :rows="2"
            placeholder="请输入模板描述"
          />
        </el-form-item>
        <el-form-item label="模板分类" prop="category">
          <el-select v-model="templateForm.category" placeholder="请选择分类">
            <el-option label="RTMP" value="rtmp" />
            <el-option label="RTSP" value="rtsp" />
            <el-option label="WebRTC" value="webrtc" />
            <el-option label="SRT" value="srt" />
            <el-option label="其他" value="other" />
          </el-select>
        </el-form-item>
        <el-form-item label="配置数据" prop="configData">
          <el-input
            v-model="templateForm.configData"
            type="textarea"
            :rows="10"
            placeholder="请输入JSON格式的配置数据"
          />
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="handleTemplateDialogClose">取消</el-button>
          <el-button type="primary" :loading="saving" @click="saveTemplate">
            {{ saving ? '保存中...' : '保存' }}
          </el-button>
        </span>
      </template>
    </el-dialog>

    <!-- 查看模板对话框 -->
    <el-dialog v-model="viewDialogVisible" title="查看模板" width="800px">
      <div v-if="currentTemplate">
        <el-descriptions :column="2" border>
          <el-descriptions-item label="模板名称">{{ currentTemplate.name }}</el-descriptions-item>
          <el-descriptions-item label="分类">{{ currentTemplate.category }}</el-descriptions-item>
          <el-descriptions-item label="创建者">{{ currentTemplate.author }}</el-descriptions-item>
          <el-descriptions-item label="创建时间">{{ formatTime(currentTemplate.createTime) }}</el-descriptions-item>
          <el-descriptions-item label="描述" :span="2">{{ currentTemplate.description }}</el-descriptions-item>
        </el-descriptions>
        <div style="margin-top: 20px;">
          <h4>配置数据</h4>
          <el-input
            :model-value="formatJson(currentTemplate.configData)"
            type="textarea"
            :rows="15"
            readonly
          />
        </div>
      </div>
    </el-dialog>

    <!-- 应用模板对话框 -->
    <el-dialog v-model="applyDialogVisible" title="应用模板" width="600px">
      <el-form :model="applyForm" label-width="100px">
        <el-form-item label="目标路径">
          <el-input v-model="applyForm.targetPath" placeholder="请输入目标流路径" />
        </el-form-item>
        <el-form-item label="应用参数">
          <el-input
            v-model="applyForm.params"
            type="textarea"
            :rows="4"
            placeholder="请输入JSON格式的应用参数（可选）"
          />
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="applyDialogVisible = false">取消</el-button>
          <el-button type="primary" :loading="applying" @click="confirmApplyTemplate">
            {{ applying ? '应用中...' : '应用' }}
          </el-button>
        </span>
      </template>
    </el-dialog>

    <!-- 导入模板对话框 -->
    <el-dialog v-model="importDialogVisible" title="导入模板" width="600px">
      <el-upload
        class="upload-demo"
        drag
        :auto-upload="false"
        :on-change="handleFileChange"
        :file-list="fileList"
        accept=".json"
      >
        <el-icon class="el-icon--upload"><upload-filled /></el-icon>
        <div class="el-upload__text">
          将文件拖到此处，或<em>点击上传</em>
        </div>
        <template #tip>
          <div class="el-upload__tip">
            只能上传 JSON 文件
          </div>
        </template>
      </el-upload>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="importDialogVisible = false">取消</el-button>
          <el-button type="primary" :loading="importing" @click="confirmImportTemplate">
            {{ importing ? '导入中...' : '导入' }}
          </el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { templateAPI } from '@/api/batch'

// 响应式数据
const loading = ref(false)
const saving = ref(false)
const applying = ref(false)
const importing = ref(false)
const templates = ref([])
const selectedTemplates = ref([])

// 搜索和过滤
const searchText = ref('')
const categoryFilter = ref('')
const typeFilter = ref('')

// 分页
const currentPage = ref(1)
const pageSize = ref(20)
const totalTemplates = ref(0)

// 对话框状态
const templateDialogVisible = ref(false)
const viewDialogVisible = ref(false)
const applyDialogVisible = ref(false)
const importDialogVisible = ref(false)
const isEditing = ref(false)
const currentTemplate = ref(null)

// 表单数据
const templateForm = ref({
  name: '',
  description: '',
  category: '',
  configData: ''
})

const applyForm = ref({
  targetPath: '',
  params: ''
})

const fileList = ref([])

// 表单验证规则
const templateRules = {
  name: [
    { required: true, message: '请输入模板名称', trigger: 'blur' }
  ],
  category: [
    { required: true, message: '请选择模板分类', trigger: 'change' }
  ],
  configData: [
    { required: true, message: '请输入配置数据', trigger: 'blur' },
    { validator: validateJson, trigger: 'blur' }
  ]
}

// 计算属性
const filteredTemplates = computed(() => {
  let result = templates.value

  // 搜索过滤
  if (searchText.value) {
    result = result.filter(template =>
      template.name.toLowerCase().includes(searchText.value.toLowerCase()) ||
      template.description.toLowerCase().includes(searchText.value.toLowerCase())
    )
  }

  // 分类过滤
  if (categoryFilter.value) {
    result = result.filter(template => template.category === categoryFilter.value)
  }

  // 类型过滤
  if (typeFilter.value) {
    if (typeFilter.value === 'default') {
      result = result.filter(template => template.isDefault)
    } else if (typeFilter.value === 'custom') {
      result = result.filter(template => !template.isDefault)
    }
  }

  totalTemplates.value = result.length

  // 分页
  const start = (currentPage.value - 1) * pageSize.value
  const end = start + pageSize.value
  return result.slice(start, end)
})

// 方法
function validateJson(rule, value, callback) {
  try {
    JSON.parse(value)
    callback()
  } catch (error) {
    callback(new Error('配置数据必须是有效的JSON格式'))
  }
}

const refreshTemplates = async () => {
  loading.value = true
  try {
    const response = await templateAPI.getTemplates()
    templates.value = response.data || []
  } catch (error) {
    ElMessage.error('获取模板列表失败: ' + error.message)
  } finally {
    loading.value = false
  }
}

const handleFilter = () => {
  currentPage.value = 1
}

const handleSelectionChange = (selection) => {
  selectedTemplates.value = selection
}

const handleSizeChange = (size) => {
  pageSize.value = size
  currentPage.value = 1
}

const handleCurrentChange = (page) => {
  currentPage.value = page
}

const getCategoryTagType = (category) => {
  const typeMap = {
    rtmp: 'primary',
    rtsp: 'success',
    webrtc: 'warning',
    srt: 'info',
    other: 'default'
  }
  return typeMap[category] || 'default'
}

const formatTime = (timestamp) => {
  if (!timestamp) return 'N/A'
  return new Date(parseInt(timestamp) * 1000).toLocaleString()
}

const formatJson = (jsonStr) => {
  try {
    return JSON.stringify(JSON.parse(jsonStr), null, 2)
  } catch (error) {
    return jsonStr
  }
}

onMounted(() => {
  refreshTemplates()
})
</script>

<style scoped>
.config-templates {
  max-width: 100%;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.header-actions {
  display: flex;
  gap: 10px;
}

.filter-section {
  margin-bottom: 20px;
}

.pagination-container {
  margin-top: 20px;
  text-align: right;
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
}
</style>
