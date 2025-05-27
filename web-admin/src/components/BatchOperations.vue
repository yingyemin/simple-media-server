<template>
  <div class="batch-operations">
    <el-dialog
      v-model="visible"
      :title="dialogTitle"
      width="600px"
      :before-close="handleClose"
    >
      <div class="operation-content">
        <!-- 操作类型选择 -->
        <el-form :model="form" label-width="100px">
          <el-form-item label="操作类型">
            <el-select v-model="form.operation" placeholder="请选择操作类型" @change="onOperationChange">
              <el-option
                v-for="op in availableOperations"
                :key="op.value"
                :label="op.label"
                :value="op.value"
                :disabled="op.disabled"
              />
            </el-select>
          </el-form-item>

          <!-- 目标列表 -->
          <el-form-item label="操作目标">
            <div class="target-list">
              <el-tag
                v-for="target in selectedTargets"
                :key="target"
                closable
                @close="removeTarget(target)"
                style="margin-right: 8px; margin-bottom: 8px;"
              >
                {{ target }}
              </el-tag>
              <div v-if="selectedTargets.length === 0" class="no-targets">
                未选择任何目标
              </div>
            </div>
          </el-form-item>

          <!-- 操作参数 -->
          <el-form-item v-if="showParams" label="操作参数">
            <el-input
              v-model="form.params"
              type="textarea"
              :rows="4"
              placeholder="请输入JSON格式的操作参数"
            />
          </el-form-item>

          <!-- 操作确认 -->
          <el-form-item v-if="form.operation" label="操作确认">
            <el-alert
              :title="getOperationWarning()"
              :type="getOperationAlertType()"
              show-icon
              :closable="false"
            />
          </el-form-item>
        </el-form>

        <!-- 操作结果 -->
        <div v-if="operationResults.length > 0" class="operation-results">
          <h4>操作结果</h4>
          <el-table :data="operationResults" style="width: 100%" max-height="300">
            <el-table-column prop="id" label="目标" width="200" />
            <el-table-column prop="success" label="状态" width="80">
              <template #default="scope">
                <el-tag :type="scope.row.success ? 'success' : 'danger'">
                  {{ scope.row.success ? '成功' : '失败' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="message" label="消息" />
          </el-table>
        </div>
      </div>

      <template #footer>
        <span class="dialog-footer">
          <el-button @click="handleClose">取消</el-button>
          <el-button
            type="primary"
            :loading="executing"
            :disabled="!canExecute"
            @click="executeOperation"
          >
            {{ executing ? '执行中...' : '执行操作' }}
          </el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, computed, watch } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { batchAPI } from '@/api/batch'

const props = defineProps({
  modelValue: {
    type: Boolean,
    default: false
  },
  type: {
    type: String,
    default: 'stream', // stream, client
    validator: (value) => ['stream', 'client'].includes(value)
  },
  targets: {
    type: Array,
    default: () => []
  }
})

const emit = defineEmits(['update:modelValue', 'success', 'error'])

// 响应式数据
const visible = computed({
  get: () => props.modelValue,
  set: (value) => emit('update:modelValue', value)
})

const form = ref({
  operation: '',
  params: ''
})

const executing = ref(false)
const operationResults = ref([])
const selectedTargets = ref([])

// 计算属性
const dialogTitle = computed(() => {
  return props.type === 'stream' ? '批量流操作' : '批量客户端操作'
})

const availableOperations = computed(() => {
  if (props.type === 'stream') {
    return [
      { label: '启动流', value: 'start', disabled: false },
      { label: '停止流', value: 'stop', disabled: false },
      { label: '删除流', value: 'delete', disabled: false }
    ]
  } else {
    return [
      { label: '断开连接', value: 'disconnect', disabled: false },
      { label: '踢出客户端', value: 'kick', disabled: false }
    ]
  }
})

const showParams = computed(() => {
  return ['config_update'].includes(form.value.operation)
})

const canExecute = computed(() => {
  return form.value.operation && selectedTargets.value.length > 0
})

// 监听器
watch(() => props.targets, (newTargets) => {
  selectedTargets.value = [...newTargets]
}, { immediate: true })

watch(() => props.modelValue, (newValue) => {
  if (newValue) {
    // 重置表单
    form.value = {
      operation: '',
      params: ''
    }
    operationResults.value = []
  }
})

// 方法
const onOperationChange = () => {
  operationResults.value = []
}

const removeTarget = (target) => {
  const index = selectedTargets.value.indexOf(target)
  if (index > -1) {
    selectedTargets.value.splice(index, 1)
  }
}

const getOperationWarning = () => {
  const count = selectedTargets.value.length
  const operationName = availableOperations.value.find(op => op.value === form.value.operation)?.label || '操作'
  
  switch (form.value.operation) {
    case 'delete':
    case 'kick':
      return `警告：此操作将对 ${count} 个目标执行"${operationName}"，操作不可撤销！`
    case 'stop':
    case 'disconnect':
      return `注意：此操作将对 ${count} 个目标执行"${operationName}"，可能影响正在进行的服务。`
    default:
      return `将对 ${count} 个目标执行"${operationName}"操作。`
  }
}

const getOperationAlertType = () => {
  switch (form.value.operation) {
    case 'delete':
    case 'kick':
      return 'error'
    case 'stop':
    case 'disconnect':
      return 'warning'
    default:
      return 'info'
  }
}

const executeOperation = async () => {
  try {
    // 危险操作需要二次确认
    if (['delete', 'kick'].includes(form.value.operation)) {
      await ElMessageBox.confirm(
        getOperationWarning(),
        '确认操作',
        {
          confirmButtonText: '确定执行',
          cancelButtonText: '取消',
          type: 'warning',
        }
      )
    }

    executing.value = true
    operationResults.value = []

    const requestData = {
      targets: selectedTargets.value,
      operation: form.value.operation,
      params: form.value.params ? JSON.parse(form.value.params) : {}
    }

    let response
    if (props.type === 'stream') {
      response = await batchAPI.batchStreamOperation(requestData)
    } else {
      response = await batchAPI.batchClientOperation(requestData)
    }

    operationResults.value = response.data.results || []
    
    const successCount = response.data.successCount || 0
    const failureCount = response.data.failureCount || 0
    
    if (failureCount === 0) {
      ElMessage.success(`批量操作完成，成功处理 ${successCount} 个目标`)
      emit('success', response.data)
    } else {
      ElMessage.warning(`批量操作完成，成功 ${successCount} 个，失败 ${failureCount} 个`)
      emit('error', response.data)
    }

  } catch (error) {
    if (error !== 'cancel') {
      console.error('批量操作失败:', error)
      ElMessage.error('批量操作失败: ' + (error.message || '未知错误'))
      emit('error', error)
    }
  } finally {
    executing.value = false
  }
}

const handleClose = () => {
  if (executing.value) {
    ElMessage.warning('操作正在执行中，请稍候...')
    return
  }
  visible.value = false
}
</script>

<style scoped>
.batch-operations {
  /* 组件样式 */
}

.operation-content {
  padding: 0;
}

.target-list {
  min-height: 60px;
  padding: 8px;
  border: 1px solid #dcdfe6;
  border-radius: 4px;
  background-color: #fafafa;
}

.no-targets {
  color: #909399;
  font-style: italic;
  text-align: center;
  padding: 20px 0;
}

.operation-results {
  margin-top: 20px;
  padding-top: 20px;
  border-top: 1px solid #ebeef5;
}

.operation-results h4 {
  margin: 0 0 15px 0;
  color: #303133;
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
}
</style>
